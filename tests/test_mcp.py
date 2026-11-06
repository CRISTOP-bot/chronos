#!/usr/bin/env python3
"""Integration test for CHRONOS MCP server using all exposed tools."""
import json, subprocess, sys, os

CLI = "/home/cristopher/chronos/build/tools/chronos_cli"
SERVER = "/home/cristopher/chronos/mcp/mcp_server.py"
REC = "/tmp/opencode/hello5.chronos"
D2 = "/tmp/opencode/d2.chronos"

env = dict(os.environ, CHRONOS_CLI=CLI)
proc = subprocess.Popen([SERVER], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env)

def send(msg):
    proc.stdin.write(json.dumps(msg) + "\n")
    proc.stdin.flush()
    line = proc.stdout.readline()
    return json.loads(line) if line else None

# initialize
r = send({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})
assert r and r["result"]["serverInfo"]["name"] == "chronos-mcp", r

# list tools
r = send({"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}})
tools = [t["name"] for t in r["result"]["tools"]]
print("TOOLS:", tools)
expected = {"chronos_info","chronos_trace","chronos_search","chronos_state","chronos_checkpoints","chronos_memdiff","chronos_export_html","chronos_strings"}
assert set(tools) == expected, set(tools)

results = {}

# chronos_info
r = send({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"chronos_info","arguments":{"recording":REC}}})
results["chronos_info"] = r["result"]["content"][0]["text"]
assert "events=" in results["chronos_info"] or "events" in results["chronos_info"]

# chronos_checkpoints
r = send({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"chronos_checkpoints","arguments":{"recording":REC}}})
results["chronos_checkpoints"] = r["result"]["content"][0]["text"]
assert "checkpoints=" in results["chronos_checkpoints"] or "Checkpoint" in results["chronos_checkpoints"]

# chronos_strings
r = send({"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"chronos_strings","arguments":{"recording":REC,"checkpoint":1,"filter":"hello"}}})
results["chronos_strings"] = r["result"]["content"][0]["text"]
assert "hello" in results["chronos_strings"]

# chronos_search
r = send({"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"chronos_search","arguments":{"recording":REC,"text":"write"}}})
results["chronos_search"] = r["result"]["content"][0]["text"]
assert "matches=" in results["chronos_search"] or "Syscall" in results["chronos_search"]

# chronos_state
r = send({"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"chronos_state","arguments":{"recording":D2,"index":3}}})
results["chronos_state"] = r["result"]["content"][0]["text"]
assert "x (int) = 42" in results["chronos_state"]

# chronos_memdiff
r = send({"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"name":"chronos_memdiff","arguments":{"recording":REC,"a":1,"b":2}}})
results["chronos_memdiff"] = r["result"]["content"][0]["text"]
assert "Comparing checkpoint" in results["chronos_memdiff"]

# chronos_export_html
out_html = "/tmp/opencode/mcp_report.html"
r = send({"jsonrpc":"2.0","id":9,"method":"tools/call","params":{"name":"chronos_export_html","arguments":{"recording":REC,"output":out_html}}})
results["chronos_export_html"] = r["result"]["content"][0]["text"]
assert os.path.exists(out_html)

# chronos_trace
out_rec = "/tmp/opencode/mcp_trace.chronos"
r = send({"jsonrpc":"2.0","id":10,"method":"tools/call","params":{"name":"chronos_trace","arguments":{"binary":"/bin/echo","output":out_rec}}})
results["chronos_trace"] = r["result"]["content"][0]["text"]
assert os.path.exists(out_rec)

proc.stdin.close()
proc.wait()

print("\n=== MCP TEST SUMMARY ===")
for k,v in results.items():
    print(f"- {k}: OK")
print("ALL TOOLS TESTED SUCCESSFULLY")