"""极简 JSON-RPC 客户端 —— 用标准库 urllib，看清每个请求/响应的原始报文。

先启动 server.py，再运行: python3 client.py
"""
import json
import urllib.request

URL = "http://127.0.0.1:8000/rpc"
_next_id = 0    # 客户端自己维护自增 id，用来匹配 "哪个响应对应哪个请求"


def call(method, params=None, *, notify=False):
    """发起一次 JSON-RPC 调用并打印原始报文"""
    global _next_id
    req = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        req["params"] = params
    if not notify:               # 通知没有 id，服务端不回复
        _next_id += 1
        req["id"] = _next_id

    raw = json.dumps(req, ensure_ascii=False).encode()
    print(f"--> 请求: {raw.decode()}")

    http_req = urllib.request.Request(
        URL, data=raw, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(http_req) as resp:
        if resp.status == 204:   # 通知无响应体
            print("<-- 通知已发送（服务端不回复）\n")
            return None
        body = resp.read().decode()
        print(f"<-- 响应: {body}\n")
        return json.loads(body)


if __name__ == "__main__":
    # 1. 位置参数 [1, 2] → 服务端执行 add(1, 2)
    call("add", [1, 2])

    # 2. 命名参数 {"a":..,"b":..} → 服务端执行 add(a=10, b=32)
    call("add", {"a": 10, "b": 32})

    # 3. 返回结构体（dict 照样能序列化传回）
    call("get_user", {"uid": 1})

    # 4. 业务错误：用户不存在 → error.code = -32603
    call("get_user", {"uid": 999})

    # 5. 方法未注册 → error.code = -32601
    call("no_this_method")

    # 6. 参数不匹配 → error.code = -32602
    call("add", {"a": 1})

    # 7. 通知（无 id）：服务端只执行不回复
    call("greet", ["老王"], notify=True)
