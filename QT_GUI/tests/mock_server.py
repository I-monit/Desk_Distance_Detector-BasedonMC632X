#!/usr/bin/env python3
"""
mock_server.py — 模拟 MC632X 板端 tcp_ctrl 服务

启动：
    python tests/mock_server.py [--port 6000] [--host 0.0.0.0]

行为：
    1. 接受单客户端连接
    2. 100ms 周期推送状态 JSON (含 ts/fid/pts/ats 帧级配对字段, 同新固件协议)
    3. 接收 KEY1 命令：进入 CALIBRATING，pairs 递增，5 对后转 READY
    4. 接收 PING 命令：回 PONG
"""

import argparse
import json
import socket
import threading
import time


class MockBoard:
    def __init__(self):
        self.state = "IDLE"
        self.pairs = 0
        self.D = -1
        self.C = -1
        self.W = -1
        self.faces = 0
        self.calibrating_until = 0.0
        # 帧级配对字段: 模拟 30fps 编码时钟(pts)与 rt_tick(ats/ts) 同步推进
        self.boot = time.monotonic()
        self.frame_id = 0

    def rt_ms(self) -> int:
        return int((time.monotonic() - self.boot) * 1000)

    def handle_cmd(self, cmd: str) -> str:
        cmd = cmd.strip().upper()
        if cmd == "KEY1":
            self.state = "CALIBRATING"
            self.pairs = 0
            self.calibrating_until = time.time() + 3.0
            return ""
        if cmd == "PING":
            return "PONG\n"
        return ""

    def tick(self):
        if self.state == "CALIBRATING":
            if time.time() > self.calibrating_until:
                self.state = "IDLE"
                self.pairs = 0
            else:
                if self.pairs < 5:
                    self.pairs += 1
                if self.pairs >= 5:
                    self.state = "READY"
                    self.D = 1234
                    self.C = 56789
                    self.W = 96
        # faces 模拟: READY 期每 20s 出现 3s 无人脸窗口 (faces=0),
        # 期间 D/W/C 冻结陈旧值持续推送 —— 复现真实板端行为, 供 PC 端无效判定联调
        if self.state == "READY":
            phase = time.time() % 20
            self.faces = 0 if 15 <= phase < 18 else 1
        else:
            self.faces = 1 if self.state == "CALIBRATING" else 0

    def snapshot(self) -> str:
        rt = self.rt_ms()
        pts = int(rt * 1.25)  # 模拟独立 VENC 时钟 (与 rt_tick 存在固定偏移+漂移)
        self.frame_id += 3    # 100ms 推送间隔 x 30fps
        ready = self.state == "READY"
        calibrating = self.state == "CALIBRATING"
        # 模拟标定期配对质量: tts/tfid/tdev/twin (Time_Stamp Phase④)
        # 合格偏差 0~50ms, 模拟 1/3 概率抖动到 60~120ms 验证 PC 端显示
        if calibrating and self.pairs > 0:
            tts  = rt - 30
            tfid = max(0, self.frame_id - 5)
            tdev = 0 if (self.pairs % 3) else 85
            twin = 560
        else:
            tts = tfid = tdev = twin = 0
        return json.dumps({
            "state": self.state,
            "D":     self.D,
            "C":     self.C,
            "W":     self.W,
            "faces": self.faces,
            "pairs": self.pairs,
            "ts":    rt - 60 if ready else 0,   # D 对应模型帧时刻 (比推送早 ~2 帧)
            "fid":   self.frame_id - 2 if ready else 0,
            "tts":   tts,
            "tfid":  tfid,
            "tdev":  tdev,
            "twin":  twin,
            "pts":   pts,
            "ats":   rt,
        }, separators=(",", ":")) + "\n"


def serve(host: str, port: int):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((host, port))
    srv.listen(1)
    print(f"[mock] listening on {host}:{port}")

    while True:
        conn, addr = srv.accept()
        print(f"[mock] client connected from {addr}")
        conn.settimeout(0.1)
        board = MockBoard()
        buf = b""
        last_tick = time.time()
        last_push = time.time()

        try:
            while True:
                now = time.time()
                if now - last_tick >= 0.1:
                    board.tick()
                    last_tick = now
                if now - last_push >= 0.1:
                    try:
                        conn.sendall(board.snapshot().encode("utf-8"))
                        last_push = now
                    except (BrokenPipeError, ConnectionResetError):
                        break

                try:
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        resp = board.handle_cmd(line.decode("utf-8", errors="ignore"))
                        if resp:
                            conn.sendall(resp.encode("utf-8"))
                except socket.timeout:
                    pass
                except (ConnectionResetError, BrokenPipeError):
                    break
        finally:
            print(f"[mock] client {addr} disconnected")
            try:
                conn.close()
            except OSError:
                pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=6000)
    args = ap.parse_args()
    serve(args.host, args.port)


if __name__ == "__main__":
    main()
