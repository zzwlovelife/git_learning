"""手写 JSON-RPC 2.0 服务器 —— 只用标准库，看清协议本质。

启动: python3 server.py   (监听 127.0.0.1:8000)
测试: python3 client.py   或用 curl (见 client.py / 会话说明)
"""
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# ============================================================
# 第一部分：方法注册表 —— 把 "方法名字符串" 映射到 "真正的函数"
# 客户端报文里的 "method": "add" 就靠它找到该执行的函数
# ============================================================
RPC_METHODS = {}

def rpc_method(func):
    """装饰器：注册一个可被远程调用的方法"""
    RPC_METHODS[func.__name__] = func
    return func

# ============================================================
# 第二部分：业务方法 —— 服务端真正干活的代码
# ============================================================
@rpc_method
def add(a, b):
    return a + b

@rpc_method
def greet(name):
    return f"你好, {name}!"

@rpc_method
def get_user(uid):
    users = {1: {"name": "Alice", "age": 18}, 2: {"name": "Bob", "age": 25}}
    if uid not in users:
        raise ValueError(f"用户 {uid} 不存在")   # 应用级业务错误
    return users[uid]

# ============================================================
# 第三部分：JSON-RPC 2.0 协议层（学习重点！）
#
# 请求报文: {"jsonrpc":"2.0", "method":"add", "params":[1,2], "id":1}
# 成功响应: {"jsonrpc":"2.0", "result":3,                  "id":1}
# 失败响应: {"jsonrpc":"2.0", "error":{"code":...,"message":...}, "id":1}
# ============================================================
PARSE_ERROR      = -32700  # JSON 语法都解析不了
INVALID_REQUEST  = -32600  # 不是合法的请求对象
METHOD_NOT_FOUND = -32601  # 方法没注册
INVALID_PARAMS   = -32602  # 参数个数/名字不对
INTERNAL_ERROR   = -32603  # 业务代码抛异常

def error_resp(req_id, code, message):
    return {"jsonrpc": "2.0", "error": {"code": code, "message": message}, "id": req_id}

def handle_request(req):
    """处理单个请求对象。

    返回响应 dict；如果是通知(notification, 报文里没有 id 字段)则返回 None，
    规范要求：通知只触发执行，服务端任何情况下都不回复（包括出错也不回复）。
    """
    # 不是 dict 或缺 method 字段 → 无效请求
    if not isinstance(req, dict) or "method" not in req:
        return error_resp(None, INVALID_REQUEST, "Invalid Request")

    is_notify = "id" not in req     # 没有 id 字段 → 通知
    req_id = req.get("id")

    def respond(resp):
        return None if is_notify else resp

    method = RPC_METHODS.get(req["method"])
    if method is None:
        return respond(error_resp(req_id, METHOD_NOT_FOUND, f"未知方法: {req['method']}"))

    params = req.get("params", [])
    try:
        # 位置参数 [1,2] → add(1,2)；命名参数 {"a":1,"b":2} → add(a=1,b=2)
        if isinstance(params, list):
            result = method(*params)
        elif isinstance(params, dict):
            result = method(**params)
        else:
            return respond(error_resp(req_id, INVALID_PARAMS, "params 必须是数组或对象"))
        return respond({"jsonrpc": "2.0", "result": result, "id": req_id})
    except TypeError as e:        # Python 层参数不匹配 ≈ 参数错误
        return respond(error_resp(req_id, INVALID_PARAMS, f"参数错误: {e}"))
    except Exception as e:        # 业务异常 → 内部错误
        return respond(error_resp(req_id, INTERNAL_ERROR, str(e)))

# ============================================================
# 第四部分：HTTP 承载层 —— JSON-RPC 与传输方式无关，
# 用 HTTP 只是最常见的载体（gRPC 用 HTTP2，还有用 WebSocket/TCP 的）
# ============================================================
class RPCHandler(BaseHTTPRequestHandler):

    def do_POST(self):
        raw = self.rfile.read(int(self.headers["Content-Length"]))
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError:
            self._send(error_resp(None, PARSE_ERROR, "Parse error"))
            return

        if isinstance(payload, list):
            # 批量请求：一次 HTTP 请求塞多个调用
            responses = [r for r in map(handle_request, payload) if r is not None]
            if responses:
                self._send(responses)
            else:                     # 全是通知 → 204 无内容
                self.send_response(204)
                self.end_headers()
        else:
            body = handle_request(payload)
            if body is None:          # 单个通知 → 204
                self.send_response(204)
                self.end_headers()
            else:
                self._send(body)

    def _send(self, obj):
        data = json.dumps(obj, ensure_ascii=False).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, fmt, *args):    # 打印请求日志便于观察
        print(f">> {fmt % args}")

if __name__ == "__main__":
    print("JSON-RPC 服务器已启动: http://127.0.0.1:8000/rpc  (Ctrl+C 退出)")
    ThreadingHTTPServer(("127.0.0.1", 8000), RPCHandler).serve_forever()
