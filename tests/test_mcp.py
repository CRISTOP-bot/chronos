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

r = send({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})
assert r and r["result"]["serverInfo"]["name"] == "chronos-mcp", r

r = send({"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}})
tools = [t["name"] for t in r["result"]["tools"]]
print("TOOLS:", tools)
expected = {"chronos_info","chronos_trace","chronos_search","chronos_state","chronos_checkpoints","chronos_memdiff","chronos_export_html","chronos_strings","chronos_reconstruct","chronos_stats","chronos_export_json","chronos_branch","chronos_project_info"}
assert set(tools) == expected, set(tools)

results = {}
def call(tid, name, args):
    r = send({"jsonrpc":"2.0","id":tid,"method":"tools/call","params":{"name":name,"arguments":args}})
    return r["result"]["content"][0]["text"]

results["chronos_info"] = call(3, "chronos_info", {"recording": REC})
assert "events=" in results["chronos_info"] or "events" in results["chronos_info"]

results["chronos_checkpoints"] = call(4, "chronos_checkpoints", {"recording": REC})
assert "checkpoints=" in results["chronos_checkpoints"] or "Checkpoint" in results["chronos_checkpoints"]

results["chronos_strings"] = call(5, "chronos_strings", {"recording": REC, "checkpoint": 1, "filter": "hello"})
assert "hello" in results["chronos_strings"]

results["chronos_search"] = call(6, "chronos_search", {"recording": REC, "text": "write"})
assert "matches=" in results["chronos_search"] or "Syscall" in results["chronos_search"]

results["chronos_state"] = call(7, "chronos_state", {"recording": D2, "index": 3})
assert "x (int) = 42" in results["chronos_state"]

results["chronos_memdiff"] = call(8, "chronos_memdiff", {"recording": REC, "a": 1, "b": 2})
assert "Comparing checkpoint" in results["chronos_memdiff"]

out_html = "/tmp/opencode/mcp_report.html"
results["chronos_export_html"] = call(9, "chronos_export_html", {"recording": REC, "output": out_html})
assert os.path.exists(out_html)

out_rec = "/tmp/opencode/mcp_trace.chronos"
results["chronos_trace"] = call(10, "chronos_trace", {"binary": "/bin/echo", "output": out_rec})
assert os.path.exists(out_rec)

results["chronos_reconstruct"] = call(11, "chronos_reconstruct", {"recording": REC, "checkpoint": 1})
assert "Checkpoint" in results["chronos_reconstruct"]

results["chronos_stats"] = call(12, "chronos_stats", {"recording": REC})
assert "Total events:" in results["chronos_stats"]

out_json = "/tmp/opencode/mcp_out.json"
results["chronos_export_json"] = call(13, "chronos_export_json", {"recording": REC, "output": out_json})
assert os.path.exists(out_json)

out_branch = "/tmp/opencode/mcp_branch.chronos"
results["chronos_branch"] = call(14, "chronos_branch", {"source": REC, "output": out_branch, "label": "mcp-branch"})
assert os.path.exists(out_branch)

proj_dir = "/tmp/opencode/mcpproj"
os.makedirs(proj_dir, exist_ok=True)
proj_file = proj_dir + "/project.json"
subprocess.run([CLI, "project-create", proj_dir, "MCPProj"], capture_output=True)
subprocess.run([CLI, "project-add", proj_file, REC], capture_output=True)
results["chronos_project_info"] = call(15, "chronos_project_info", {"project": proj_file})
assert "Project:" in results["chronos_project_info"]

proc.stdin.close()
proc.wait()

print("\n=== MCP TEST SUMMARY ===")
for k in results:
    print(f"- {k}: OK")
print("ALL TOOLS TESTED SUCCESSFULLY")