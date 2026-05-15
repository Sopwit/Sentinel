import { useEffect, useRef } from "react";

/**
 * Volumetric neural point-cloud entity.
 * Pure canvas — particles orbit around a soft core, breathe, and react to pointer drift.
 */
export function AICore({ size = 420 }: { size?: number }) {
  const ref = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = ref.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = size * dpr;
    canvas.height = size * dpr;
    ctx.scale(dpr, dpr);

    const cx = size / 2;
    const cy = size / 2;
    const N = 520;

    type P = { theta: number; phi: number; r: number; speed: number; tilt: number };
    const points: P[] = Array.from({ length: N }, () => ({
      theta: Math.random() * Math.PI * 2,
      phi: Math.acos(2 * Math.random() - 1),
      r: 110 + Math.random() * 40,
      speed: 0.0008 + Math.random() * 0.0014,
      tilt: Math.random() * 0.4 - 0.2,
    }));

    let mouseX = 0, mouseY = 0, tx = 0, ty = 0;
    const onMove = (e: MouseEvent) => {
      const rect = canvas.getBoundingClientRect();
      tx = ((e.clientX - rect.left) / rect.width - 0.5) * 0.6;
      ty = ((e.clientY - rect.top) / rect.height - 0.5) * 0.6;
    };
    window.addEventListener("mousemove", onMove);

    let raf = 0;
    let t = 0;
    const render = () => {
      t += 1;
      mouseX += (tx - mouseX) * 0.03;
      mouseY += (ty - mouseY) * 0.03;

      ctx.clearRect(0, 0, size, size);

      // Soft core glow
      const breathing = 1 + Math.sin(t * 0.012) * 0.05;
      const grd = ctx.createRadialGradient(cx, cy, 0, cx, cy, 150 * breathing);
      grd.addColorStop(0, "rgba(180, 230, 255, 0.55)");
      grd.addColorStop(0.35, "rgba(120, 190, 240, 0.22)");
      grd.addColorStop(1, "rgba(20, 40, 80, 0)");
      ctx.fillStyle = grd;
      ctx.beginPath();
      ctx.arc(cx, cy, 180, 0, Math.PI * 2);
      ctx.fill();

      // Inner sphere wireframe-ish dots
      const yaw = mouseX * 1.2 + t * 0.0015;
      const pitch = mouseY * 1.0;

      const projected: { x: number; y: number; z: number; a: number }[] = [];

      for (const p of points) {
        p.theta += p.speed;
        const r = p.r * (1 + Math.sin(t * 0.01 + p.phi) * 0.02);
        const x0 = r * Math.sin(p.phi) * Math.cos(p.theta);
        const y0 = r * Math.cos(p.phi);
        const z0 = r * Math.sin(p.phi) * Math.sin(p.theta);

        // yaw
        const x1 = x0 * Math.cos(yaw) - z0 * Math.sin(yaw);
        const z1 = x0 * Math.sin(yaw) + z0 * Math.cos(yaw);
        // pitch
        const y2 = y0 * Math.cos(pitch) - z1 * Math.sin(pitch);
        const z2 = y0 * Math.sin(pitch) + z1 * Math.cos(pitch);

        const persp = 320 / (320 + z2);
        projected.push({
          x: cx + x1 * persp,
          y: cy + y2 * persp,
          z: z2,
          a: 0.35 + persp * 0.55,
        });
      }

      // Neural lines between near neighbors (sparse)
      ctx.lineWidth = 0.5;
      for (let i = 0; i < projected.length; i += 4) {
        const a = projected[i];
        for (let j = i + 1; j < Math.min(i + 14, projected.length); j++) {
          const b = projected[j];
          const dx = a.x - b.x;
          const dy = a.y - b.y;
          const d2 = dx * dx + dy * dy;
          if (d2 < 900) {
            const op = (1 - d2 / 900) * 0.18 * Math.min(a.a, b.a);
            ctx.strokeStyle = `rgba(170, 220, 255, ${op})`;
            ctx.beginPath();
            ctx.moveTo(a.x, a.y);
            ctx.lineTo(b.x, b.y);
            ctx.stroke();
          }
        }
      }

      // Points
      for (const p of projected) {
        ctx.fillStyle = `rgba(210, 240, 255, ${p.a})`;
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.z > 0 ? 1.1 : 1.6, 0, Math.PI * 2);
        ctx.fill();
      }

      raf = requestAnimationFrame(render);
    };
    render();

    return () => {
      cancelAnimationFrame(raf);
      window.removeEventListener("mousemove", onMove);
    };
  }, [size]);

  return (
    <div className="relative" style={{ width: size, height: size }}>
      {/* outer halos */}
      <div className="absolute inset-0 rounded-full animate-pulse-ring" style={{
        background: "radial-gradient(circle, transparent 58%, rgba(140, 210, 255, 0.18) 60%, transparent 62%)"
      }} />
      <div className="absolute inset-0 rounded-full animate-pulse-ring" style={{
        animationDelay: "1.5s",
        background: "radial-gradient(circle, transparent 58%, rgba(140, 210, 255, 0.12) 60%, transparent 62%)"
      }} />
      <canvas ref={ref} style={{ width: size, height: size }} className="relative animate-breathe" />
    </div>
  );
}
