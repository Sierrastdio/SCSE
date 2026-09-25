#!/usr/bin/env python3
"""Dependency-free Tkinter viewer for NEONspasim JSON snapshots."""

from __future__ import annotations

import argparse
import json
import math
import tkinter as tk
from pathlib import Path
from tkinter import ttk


TEAM_COLORS = ["#35a9ff", "#ff5d67", "#b18aff", "#ffc34d"]


class DebugViewer:
    def __init__(self, path: Path, watch: bool, interval_ms: int, horizon: float) -> None:
        self.path = path
        self.watch = watch
        self.interval_ms = interval_ms
        self.horizon = horizon
        self.plane = "iso"
        self.camera_azimuth = -55.0
        self.camera_elevation = 28.0
        self.drag_origin: tuple[int, int] | None = None
        self.drag_mode: str | None = None
        self.pan_x = 0.0
        self.pan_y = 0.0
        self.zoom = 1.0
        self.trails: dict[int, list[tuple[float, float, float]]] = {}
        self.snapshot: dict = {"tick": 0, "time": 0, "entities": [], "planets": []}

        self.root = tk.Tk()
        self.root.title("SPASIM debug renderer")
        self.root.geometry("1100x760")
        self.root.configure(bg="#15191f")

        toolbar = ttk.Frame(self.root, padding=6)
        toolbar.pack(fill=tk.X)
        self.hud = ttk.Label(toolbar, text="Waiting for snapshot", anchor=tk.W)
        self.hud.pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(toolbar, text="Orbit (1)", command=lambda: self.set_plane("iso")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="XY (2)", command=lambda: self.set_plane("xy")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="XZ (3)", command=lambda: self.set_plane("xz")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="YZ (4)", command=lambda: self.set_plane("yz")).pack(side=tk.LEFT, padx=2)

        self.canvas = tk.Canvas(self.root, bg="#10141a", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Configure>", lambda _event: self.redraw())
        self.canvas.bind("<ButtonPress-3>", self.begin_drag)
        self.canvas.bind("<B3-Motion>", self.drag)
        self.canvas.bind("<ButtonRelease-3>", self.end_drag)
        self.root.bind("1", lambda _event: self.set_plane("iso"))
        self.root.bind("2", lambda _event: self.set_plane("xy"))
        self.root.bind("3", lambda _event: self.set_plane("xz"))
        self.root.bind("4", lambda _event: self.set_plane("yz"))
        self.root.bind("<MouseWheel>", self.on_wheel)
        self.root.bind("<Button-4>", lambda _event: self.zoom_by(1.1))
        self.root.bind("<Button-5>", lambda _event: self.zoom_by(1 / 1.1))
        self.root.bind("+", lambda _event: self.zoom_by(1.1))
        self.root.bind("-", lambda _event: self.zoom_by(1 / 1.1))

        self.refresh()

    def set_plane(self, plane: str) -> None:
        self.plane = plane
        self.redraw()

    def zoom_by(self, factor: float) -> None:
        self.zoom = min(max(self.zoom * factor, 0.15), 8.0)
        self.redraw()

    def on_wheel(self, event: tk.Event) -> None:
        self.zoom_by(1.1 if event.delta > 0 else 1 / 1.1)

    def begin_drag(self, event: tk.Event) -> None:
        self.drag_origin = (event.x, event.y)
        self.drag_mode = "pan" if event.state & 0x0001 else "orbit"

    def drag(self, event: tk.Event) -> None:
        if self.drag_origin is None:
            return
        old_x, old_y = self.drag_origin
        delta_x = event.x - old_x
        delta_y = event.y - old_y
        if self.drag_mode == "pan":
            self.pan_x += delta_x
            self.pan_y += delta_y
        else:
            self.camera_azimuth = (self.camera_azimuth + delta_x * 0.5) % 360.0
            self.camera_elevation = min(89.0, max(-89.0, self.camera_elevation - delta_y * 0.5))
        self.drag_origin = (event.x, event.y)
        self.redraw()

    def end_drag(self, _event: tk.Event) -> None:
        self.drag_origin = None
        self.drag_mode = None

    def refresh(self) -> None:
        try:
            with self.path.open("r", encoding="utf-8") as source:
                snapshot = json.load(source)
            self.snapshot = snapshot
            for entity in snapshot.get("entities", []):
                if entity.get("kind") != "ship":
                    continue
                trail = self.trails.setdefault(int(entity["id"]), [])
                trail.append((entity["x"], entity["y"], entity["z"]))
                del trail[:-180]
            self.hud.configure(text=f"t={snapshot.get('time', 0):.1f}s   tick={snapshot.get('tick', 0)}   "
                                     f"objects={len(snapshot.get('entities', []))}   projection={self.plane}")
            self.redraw()
        except (OSError, json.JSONDecodeError, KeyError, TypeError) as error:
            self.hud.configure(text=f"Waiting for a valid snapshot: {error}")
        if self.watch:
            self.root.after(self.interval_ms, self.refresh)

    def project(self, point: tuple[float, float, float], center: tuple[float, float, float], scale: float) -> tuple[float, float]:
        x, y, z = (point[index] - center[index] for index in range(3))
        if self.plane == "xy":
            return x * scale, -y * scale
        if self.plane == "xz":
            return x * scale, -z * scale
        if self.plane == "yz":
            return y * scale, -z * scale
        azimuth = math.radians(self.camera_azimuth)
        elevation = math.radians(self.camera_elevation)
        right = (math.cos(azimuth), math.sin(azimuth), 0.0)
        up = (-math.sin(azimuth) * math.sin(elevation),
              math.cos(azimuth) * math.sin(elevation), math.cos(elevation))
        screen_x = x * right[0] + y * right[1] + z * right[2]
        screen_y = -(x * up[0] + y * up[1] + z * up[2])
        return screen_x * scale, screen_y * scale

    def redraw(self) -> None:
        if not hasattr(self, "canvas"):
            return
        canvas = self.canvas
        canvas.delete("all")
        width = max(canvas.winfo_width(), 100)
        height = max(canvas.winfo_height(), 100)
        entities = self.snapshot.get("entities", [])
        planets = self.snapshot.get("planets", [])
        positions = [(obj["x"], obj["y"], obj["z"]) for obj in [*entities, *planets]]
        if not positions:
            positions = [(0.0, 0.0, 0.0)]
        minimum = [min(point[axis] for point in positions) for axis in range(3)]
        maximum = [max(point[axis] for point in positions) for axis in range(3)]
        center = tuple((minimum[axis] + maximum[axis]) / 2.0 for axis in range(3))
        span = max(maximum[axis] - minimum[axis] for axis in range(3))
        scale = min(width, height) * 0.72 / max(span, 250.0) * self.zoom
        origin = (width * 0.5 + self.pan_x, height * 0.54 + self.pan_y)

        def screen(point: tuple[float, float, float]) -> tuple[float, float]:
            dx, dy = self.project(point, center, scale)
            return origin[0] + dx, origin[1] + dy

        self.draw_axes(screen, center, scale, width, height)

        for planet in planets:
            point = (planet["x"], planet["y"], planet["z"])
            orbit_radius = float(planet.get("orbit_radius", 0.0))
            if orbit_radius > 0.0:
                ring = []
                for index in range(49):
                    angle = 2.0 * math.pi * index / 48
                    ring.extend(screen((point[0] + orbit_radius * math.cos(angle),
                                        point[1] + orbit_radius * math.sin(angle), point[2])))
                canvas.create_line(*ring, fill="#74764f", dash=(3, 4), smooth=True, width=1)
            sx, sy = screen(point)
            canvas.create_oval(sx - 8, sy - 8, sx + 8, sy + 8, fill="#e8e36a", outline="white", width=1)
            canvas.create_text(sx + 12, sy - 10, text=planet.get("name", "planet"), fill="#f4ef9a", anchor=tk.W)

        for entity in entities:
            point = (entity["x"], entity["y"], entity["z"])
            sx, sy = screen(point)
            kind = entity.get("kind", "ship")
            team = int(entity.get("team", 0)) % len(TEAM_COLORS)
            color = TEAM_COLORS[team]
            if kind == "ship":
                self.draw_ship(entity, point, screen, color)
                if all(key in entity for key in ("vx", "vy", "vz")):
                    velocity = (entity["vx"], entity["vy"], entity["vz"])
                    speed = math.sqrt(sum(component * component for component in velocity))
                    if speed > 0.01:
                        arrow_length = min(max(speed * 8.0, 20.0), 100.0)
                        velocity_point = tuple(point[axis] + velocity[axis] * arrow_length / speed
                                               for axis in range(3))
                        ex, ey = screen(velocity_point)
                        canvas.create_line(sx, sy, ex, ey, fill="#f4d36a", arrow=tk.LAST, width=2)
            elif kind == "base":
                self.draw_square(sx, sy, 7, color)
            else:
                canvas.create_oval(sx - 4, sy - 4, sx + 4, sy + 4, fill="#ffffff", outline=color, width=2)
                if all(key in entity for key in ("vx", "vy", "vz")):
                    velocity_point = (point[0] + entity["vx"] * 0.3,
                                      point[1] + entity["vy"] * 0.3,
                                      point[2] + entity["vz"] * 0.3)
                    ex, ey = screen(velocity_point)
                    canvas.create_line(sx, sy, ex, ey, fill=color, arrow=tk.LAST, width=2)
            canvas.create_text(sx + 10, sy + 10, text=f"{kind} #{entity.get('id')} T{team + 1}",
                               fill=color, anchor=tk.NW, font=("Consolas", 9))

        canvas.create_text(14, height - 16, anchor=tk.SW,
                           text="Right-drag orbit · Shift+right-drag pan · dashed=facing · yellow=velocity   Scroll/+/− zoom   1-4 views",
                           fill="#b4bdc9", font=("Consolas", 9))

    def draw_axes(self, screen, center, scale: float, width: int, height: int) -> None:
        canvas = self.canvas
        axis_length = max(250.0, 0.35 * min(width, height) / max(scale, 1e-9))
        start = (0.0, 0.0, 0.0)
        sx, sy = screen(start)
        canvas.create_oval(sx - 3, sy - 3, sx + 3, sy + 3, fill="#ffffff", outline="")
        for axis, vector, color in (("X", (axis_length, 0, 0), "#ff6868"),
                                    ("Y", (0, axis_length, 0), "#70df91"),
                                    ("Z", (0, 0, axis_length), "#6eaaff")):
            ex, ey = screen(vector)
            canvas.create_line(sx, sy, ex, ey, fill=color, arrow=tk.LAST, width=2)
            canvas.create_text(ex + 5, ey - 5, text=axis, fill=color, anchor=tk.SW,
                               font=("Consolas", 10, "bold"))

        if self.plane == "xy":
            grid_axes = ((axis_length * value, axis_length * -value, 0) for value in (-1, -0.5, 0.5, 1))
        elif self.plane == "xz":
            grid_axes = ((axis_length * value, 0, axis_length * -value) for value in (-1, -0.5, 0.5, 1))
        elif self.plane == "yz":
            grid_axes = ((0, axis_length * value, axis_length * -value) for value in (-1, -0.5, 0.5, 1))
        else:
            grid_axes = ((axis_length * value, -axis_length * value, 0) for value in (-1, -0.5, 0.5, 1))
        for point in grid_axes:
            ex, ey = screen(point)
            canvas.create_line(sx, sy, ex, ey, fill="#39414c", dash=(2, 5), width=1)

    def draw_square(self, x: float, y: float, half: float, color: str) -> None:
        self.canvas.create_rectangle(x - half, y - half, x + half, y + half,
                                     fill=color, outline="white", width=1)

    def draw_ship(self, entity: dict, point: tuple[float, float, float], screen, color: str) -> None:
        sx, sy = screen(point)
        self.canvas.create_polygon(sx, sy - 8, sx - 6, sy + 6, sx + 6, sy + 6,
                                  fill=color, outline="white", width=1)
        yaw = math.radians(entity.get("yaw", 0.0))
        pitch = math.radians(entity.get("pitch", 0.0))
        speed = max(float(entity.get("speed", 0.0)), 1.0)
        distance = min(max(speed * self.horizon, 80.0), 600.0)
        future = (point[0] + math.cos(pitch) * math.cos(yaw) * distance,
                  point[1] + math.cos(pitch) * math.sin(yaw) * distance,
                  point[2] + math.sin(pitch) * distance)
        ex, ey = screen(future)
        self.canvas.create_line(sx, sy, ex, ey, fill=color, arrow=tk.LAST,
                                arrowshape=(10, 12, 4), width=2, dash=(5, 3))
        trail = self.trails.get(int(entity["id"]), [])
        if len(trail) > 1:
            coords = []
            for historic in trail:
                coords.extend(screen(historic))
            self.canvas.create_line(*coords, fill=color, width=2, smooth=True)

    def run(self) -> None:
        self.root.mainloop()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", nargs="?", type=Path, default=Path("debug/world.json"))
    parser.add_argument("--watch", action="store_true", help="reload snapshots while the game is running")
    parser.add_argument("--interval", type=int, default=200, help="refresh interval in milliseconds")
    parser.add_argument("--horizon", type=float, default=10.0, help="seconds represented by each direction line")
    args = parser.parse_args()
    if args.interval < 50:
        parser.error("--interval must be at least 50 ms")
    DebugViewer(args.snapshot, args.watch, args.interval, args.horizon).run()


if __name__ == "__main__":
    main()
