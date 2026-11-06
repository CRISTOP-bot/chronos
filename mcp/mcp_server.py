#!/usr/bin/env python3
"""Minimal MCP (Model Context Protocol) server for CHRONOS over stdio JSON-RPC."""

import json
import os
import subprocess
import sys

CHRONOS_CLI = os.environ.get("CHRONOS_CLI", "chronos_cli")

TOOLS = [
    {
        "name": "chronos_info",
        "description": "Show recording stats (events, checkpoints).",
        "inputSchema": {
            "type": "object",
            "properties": {"recording": {"type": "string"}},
            "required": ["recording"],
        },
    },
    {
        "name": "chronos_trace",
        "description": "Trace a binary with CHRONOS ptrace engine.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string"},
                "output": {"type": "string"},
            },
            "required": ["binary", "output"],
        },
    },
    {
        "name": "chronos_search",
        "description": "Search events in a recording by text.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "text": {"type": "string"},
            },
            "required": ["recording", "text"],
        },
    },
    {
        "name": "chronos_state",
        "description": "Reconstruct variable state at an event index.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "index": {"type": "integer"},
            },
            "required": ["recording", "index"],
        },
    },
    {
        "name": "chronos_checkpoints",
        "description": "List checkpoints in a recording.",
        "inputSchema": {
            "type": "object",
            "properties": {"recording": {"type": "string"}},
            "required": ["recording"],
        },
    },
    {
        "name": "chronos_memdiff",
        "description": "Compare memory between two checkpoints.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "a": {"type": "integer"},
                "b": {"type": "integer"},
            },
            "required": ["recording", "a", "b"],
        },
    },
    {
        "name": "chronos_export_html",
        "description": "Export an HTML report for a recording.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "output": {"type": "string"},
            },
            "required": ["recording", "output"],
        },
    },
    {
        "name": "chronos_strings",
        "description": "Extract readable strings from a checkpoint.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "checkpoint": {"type": "integer"},
                "filter": {"type": "string"},
            },
            "required": ["recording", "checkpoint"],
        },
    },
    {
        "name": "chronos_reconstruct",
        "description": "Reconstruct memory regions from a checkpoint.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "checkpoint": {"type": "integer"},
            },
            "required": ["recording", "checkpoint"],
        },
    },
    {
        "name": "chronos_stats",
        "description": "Compute execution metrics for a recording.",
        "inputSchema": {
            "type": "object",
            "properties": {"recording": {"type": "string"}},
            "required": ["recording"],
        },
    },
    {
        "name": "chronos_export_json",
        "description": "Export a recording to JSON.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "recording": {"type": "string"},
                "output": {"type": "string"},
            },
            "required": ["recording", "output"],
        },
    },
    {
        "name": "chronos_branch",
        "description": "Create an analytical branch from a recording.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "source": {"type": "string"},
                "output": {"type": "string"},
                "label": {"type": "string"},
            },
            "required": ["source", "output", "label"],
        },
    },
    {
        "name": "chronos_project_info",
        "description": "Show project info.",
        "inputSchema": {
            "type": "object",
            "properties": {"project": {"type": "string"}},
            "required": ["project"],
        },
    },
]


def run_cli(args):
    try:
        r = subprocess.run([CHRONOS_CLI] + args, capture_output=True, text=True, timeout=120)
        out = r.stdout
        if r.returncode != 0:
            out += "\nSTDERR:\n" + r.stderr
        return out.strip()
    except Exception as e:
        return f"Error: {e}"


def handle(msg):
    method = msg.get("method")
    mid = msg.get("id")
    # Notifications (no id) — acknowledge silently
    if mid is None:
        if method == "notifications/initialized":
            return None  # no response for notifications
        return None
    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": mid,
            "result": {
                "protocolVersion": "2024-11-05",
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "chronos-mcp", "version": "0.1.0"},
            },
        }
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": mid, "result": {"tools": TOOLS}}
    if method == "tools/call":
        params = msg.get("params", {})
        name = params.get("name")
        args = params.get("arguments", {})
        if name == "chronos_info":
            text = run_cli(["info", args["recording"]])
        elif name == "chronos_trace":
            text = run_cli(["trace", args["binary"], args["output"]])
        elif name == "chronos_search":
            text = run_cli(["search", args["recording"], args["text"]])
        elif name == "chronos_state":
            text = run_cli(["state", args["recording"], str(args["index"])])
        elif name == "chronos_checkpoints":
            text = run_cli(["checkpoints", args["recording"]])
        elif name == "chronos_memdiff":
            text = run_cli(["memdiff", args["recording"], str(args["a"]), str(args["b"])])
        elif name == "chronos_export_html":
            text = run_cli(["export-html", args["recording"], args["output"]])
        elif name == "chronos_strings":
            cmd = ["strings", args["recording"], str(args["checkpoint"])]
            if args.get("filter"):
                cmd.append(args["filter"])
            text = run_cli(cmd)
        elif name == "chronos_reconstruct":
            text = run_cli(["reconstruct", args["recording"], str(args["checkpoint"])])
        elif name == "chronos_stats":
            text = run_cli(["stats", args["recording"]])
        elif name == "chronos_export_json":
            text = run_cli(["export-json", args["recording"], args["output"]])
        elif name == "chronos_branch":
            text = run_cli(["branch", args["source"], args["output"], args["label"]])
        elif name == "chronos_project_info":
            text = run_cli(["project-info", args["project"]])
        else:
            text = f"Unknown tool: {name}"
        return {
            "jsonrpc": "2.0",
            "id": mid,
            "result": {"content": [{"type": "text", "text": text}]},
        }
    return {"jsonrpc": "2.0", "id": mid, "error": {"code": -32601, "message": "Method not found"}}


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except Exception:
            continue
        resp = handle(msg)
        if resp is not None:
            sys.stdout.write(json.dumps(resp) + "\n")
            sys.stdout.flush()


if __name__ == "__main__":
    main()
