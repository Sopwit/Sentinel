/** Ambient particle + grid + light atmosphere layered behind the UI. */
export function Atmosphere() {
  const particles = Array.from({ length: 36 });
  return (
    <div className="pointer-events-none fixed inset-0 overflow-hidden -z-10">
      <div className="absolute inset-0 grid-mesh opacity-60" />
      <div className="absolute inset-0 noise opacity-[0.35] mix-blend-overlay" />
      {/* aurora light blobs */}
      <div className="absolute -top-40 -left-32 h-[520px] w-[520px] rounded-full blur-3xl opacity-40"
           style={{ background: "radial-gradient(circle, oklch(0.55 0.16 215 / 0.7), transparent 70%)" }} />
      <div className="absolute top-1/3 -right-40 h-[600px] w-[600px] rounded-full blur-3xl opacity-30"
           style={{ background: "radial-gradient(circle, oklch(0.50 0.18 260 / 0.6), transparent 70%)" }} />
      <div className="absolute -bottom-40 left-1/3 h-[500px] w-[500px] rounded-full blur-3xl opacity-30"
           style={{ background: "radial-gradient(circle, oklch(0.55 0.14 195 / 0.6), transparent 70%)" }} />
      {/* drifting particles */}
      {particles.map((_, i) => (
        <span
          key={i}
          className="absolute rounded-full animate-drift"
          style={{
            left: `${(i * 137) % 100}%`,
            top: `${(i * 53) % 100}%`,
            width: `${1 + (i % 3)}px`,
            height: `${1 + (i % 3)}px`,
            background: "rgba(190, 225, 255, 0.5)",
            boxShadow: "0 0 8px rgba(170, 215, 255, 0.6)",
            animationDelay: `${(i % 9) * 0.7}s`,
            animationDuration: `${7 + (i % 6)}s`,
          }}
        />
      ))}
    </div>
  );
}
