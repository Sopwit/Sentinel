import { createFileRoute } from "@tanstack/react-router";
import { Shell } from "@/components/sentinel/Shell";
import { Panel } from "@/components/sentinel/Panel";
import { AICore } from "@/components/sentinel/AICore";
import { ArrowUpRight, Cpu, Database, Radio, Waves, Zap } from "lucide-react";

export const Route = createFileRoute("/")({
  component: Dashboard,
  head: () => ({
    meta: [
      { title: "Sentinel — Operating Layer" },
      { name: "description", content: "An ambient AI operating environment. A living intelligence, calm and atmospheric." },
    ],
  }),
});

const METRICS = [
  { label: "Cognition", value: "94.2", unit: "%", icon: Cpu, trend: "+0.8" },
  { label: "Throughput", value: "12.4", unit: "k/s", icon: Zap, trend: "+312" },
  { label: "Memory Index", value: "2.18", unit: "M", icon: Database, trend: "+1,204" },
  { label: "Signal", value: "-42", unit: "dBm", icon: Radio, trend: "stable" },
];

const STREAMS = [
  { t: "00:14:02", msg: "Resolved contextual ambiguity in cluster 7 · weighted toward inference path β" },
  { t: "00:13:48", msg: "Synthesized 4 conceptual bridges between memory shards 1184 and 2207" },
  { t: "00:13:31", msg: "Agent ‘Atlas’ completed perimeter scan · no anomalies in semantic field" },
  { t: "00:13:12", msg: "Re-balanced attention weights across 14 active dialogues" },
  { t: "00:12:55", msg: "Drafted 3 candidate replies · awaiting alignment confirmation" },
];

function Dashboard() {
  return (
    <Shell title="Good evening, Operator" subtitle="The system is calm. Six processes breathing in coherence.">
      <div className="grid grid-cols-12 gap-6">
        {/* Core */}
        <Panel className="col-span-12 lg:col-span-8 min-h-[560px] flex items-center justify-center" label="Core / Sentinel.Ψ">
          <div className="relative flex flex-col items-center justify-center w-full py-10">
            {/* orbiting glyphs */}
            <div className="absolute inset-0 flex items-center justify-center">
              <div className="relative h-[480px] w-[480px] animate-orbit">
                {[0, 90, 180, 270].map(a => (
                  <span key={a} className="absolute left-1/2 top-1/2 h-[460px] w-[460px] -translate-x-1/2 -translate-y-1/2 rounded-full border border-primary/10" style={{ transform: `translate(-50%,-50%) rotate(${a}deg)` }} />
                ))}
                <span className="absolute left-1/2 -translate-x-1/2 top-0 h-2 w-2 rounded-full bg-primary shadow-[0_0_16px_currentColor]" />
              </div>
            </div>
            <div className="absolute inset-0 flex items-center justify-center">
              <div className="relative h-[360px] w-[360px] animate-orbit-rev rounded-full border border-primary/5">
                <span className="absolute left-0 top-1/2 -translate-y-1/2 h-1.5 w-1.5 rounded-full bg-accent shadow-[0_0_12px_currentColor]" />
                <span className="absolute right-0 top-1/2 -translate-y-1/2 h-1 w-1 rounded-full bg-primary/80" />
              </div>
            </div>

            <AICore size={420} />

            <div className="mt-2 text-center space-y-2">
              <div className="text-[10px] font-mono uppercase tracking-[0.4em] text-primary">Coherence 0.974</div>
              <div className="text-sm text-muted-foreground max-w-md">
                "I am listening across twelve channels. Nothing demands your attention."
              </div>
            </div>

            {/* corner readouts */}
            <div className="absolute top-6 left-6 text-[10px] font-mono text-muted-foreground space-y-1">
              <div>NODE · 04A</div>
              <div className="text-primary/80">SYNCED</div>
            </div>
            <div className="absolute top-6 right-6 text-[10px] font-mono text-muted-foreground text-right space-y-1">
              <div>LAT · 12ms</div>
              <div>TEMP · 41.2°</div>
            </div>
            <div className="absolute bottom-6 left-6 text-[10px] font-mono text-muted-foreground space-y-1">
              <div>Ψ · 7.318</div>
            </div>
            <div className="absolute bottom-6 right-6 text-[10px] font-mono text-muted-foreground text-right space-y-1">
              <div>UPTIME · 412d</div>
            </div>
          </div>
        </Panel>

        {/* Metrics column */}
        <div className="col-span-12 lg:col-span-4 grid grid-cols-2 gap-4 content-start">
          {METRICS.map((m) => (
            <Panel key={m.label} className="p-5">
              <div className="flex items-center justify-between text-muted-foreground">
                <m.icon className="h-3.5 w-3.5" strokeWidth={1.4} />
                <span className="text-[10px] font-mono uppercase tracking-widest">{m.trend}</span>
              </div>
              <div className="mt-6 flex items-baseline gap-1">
                <span className="text-3xl font-light tracking-tight">{m.value}</span>
                <span className="text-xs text-muted-foreground font-mono">{m.unit}</span>
              </div>
              <div className="mt-1 text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground">{m.label}</div>
              <Sparkline />
            </Panel>
          ))}

          <Panel className="col-span-2 p-5" label="Signal Field">
            <div className="px-5 pb-5">
              <Waveform />
              <div className="mt-3 flex items-center justify-between text-[10px] font-mono text-muted-foreground">
                <span className="flex items-center gap-2"><Waves className="h-3 w-3 text-primary" /> Ambient · harmonic</span>
                <span>432 Hz</span>
              </div>
            </div>
          </Panel>
        </div>

        {/* Streams */}
        <Panel className="col-span-12 lg:col-span-7 p-0" label="Cognition Stream" corner={
          <span className="text-[10px] font-mono text-muted-foreground/70">live · 5 / 1,284</span>
        }>
          <div className="px-5 pb-5 pt-3 space-y-1">
            {STREAMS.map((s, i) => (
              <div key={i} className="group flex items-start gap-4 py-2.5 border-b border-white/5 last:border-0">
                <span className="text-[10px] font-mono text-muted-foreground/70 mt-1 w-16">{s.t}</span>
                <span className="mt-1.5 h-1 w-1 rounded-full bg-primary/60 group-hover:bg-primary group-hover:shadow-[0_0_8px_currentColor] transition" />
                <span className="text-sm text-foreground/90 flex-1">{s.msg}</span>
                <ArrowUpRight className="h-3.5 w-3.5 text-muted-foreground/40 group-hover:text-primary transition" />
              </div>
            ))}
          </div>
        </Panel>

        {/* Agents glance */}
        <Panel className="col-span-12 lg:col-span-5 p-5" label="Active Agents">
          <div className="px-5 pb-5 space-y-4">
            {[
              { name: "Atlas", task: "Perimeter cognition scan", load: 62 },
              { name: "Orin", task: "Synthesizing brief · Q4 horizon", load: 88 },
              { name: "Vela", task: "Listening · ambient signals", load: 24 },
              { name: "Kaze", task: "Indexing memory shards", load: 47 },
            ].map((a) => (
              <div key={a.name} className="flex items-center gap-4">
                <div className="relative h-9 w-9 rounded-full glass flex items-center justify-center">
                  <span className="absolute inset-0 rounded-full border border-primary/20 animate-orbit" />
                  <span className="text-[10px] font-mono text-primary">{a.name[0]}</span>
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-center justify-between">
                    <span className="text-sm">{a.name}</span>
                    <span className="text-[10px] font-mono text-muted-foreground">{a.load}%</span>
                  </div>
                  <div className="text-xs text-muted-foreground truncate">{a.task}</div>
                  <div className="mt-2 h-[2px] w-full bg-white/5 rounded overflow-hidden">
                    <div className="h-full bg-gradient-to-r from-primary/60 to-accent shadow-[0_0_8px_oklch(0.78_0.14_210)]" style={{ width: `${a.load}%` }} />
                  </div>
                </div>
              </div>
            ))}
          </div>
        </Panel>
      </div>
    </Shell>
  );
}

function Sparkline() {
  const pts = [12, 14, 11, 15, 13, 17, 14, 18, 16, 19, 17, 20, 18, 22, 19, 23];
  const w = 140, h = 28;
  const max = Math.max(...pts), min = Math.min(...pts);
  const path = pts.map((p, i) => {
    const x = (i / (pts.length - 1)) * w;
    const y = h - ((p - min) / (max - min)) * h;
    return `${i === 0 ? "M" : "L"}${x},${y}`;
  }).join(" ");
  return (
    <svg viewBox={`0 0 ${w} ${h}`} className="mt-3 w-full h-7 overflow-visible">
      <defs>
        <linearGradient id="sg" x1="0" x2="0" y1="0" y2="1">
          <stop offset="0%" stopColor="oklch(0.86 0.10 210)" stopOpacity="0.6" />
          <stop offset="100%" stopColor="oklch(0.86 0.10 210)" stopOpacity="0" />
        </linearGradient>
      </defs>
      <path d={`${path} L${w},${h} L0,${h} Z`} fill="url(#sg)" />
      <path d={path} fill="none" stroke="oklch(0.88 0.13 200)" strokeWidth="1.2" />
    </svg>
  );
}

function Waveform() {
  const bars = Array.from({ length: 64 });
  return (
    <div className="flex items-center gap-[3px] h-12">
      {bars.map((_, i) => {
        const h = 10 + (Math.sin(i * 0.55) * 0.5 + 0.5) * 80 + (i % 7) * 4;
        return (
          <span
            key={i}
            className="flex-1 rounded-full bg-gradient-to-t from-primary/30 to-accent animate-breathe"
            style={{ height: `${Math.min(100, h)}%`, animationDelay: `${i * 0.04}s`, animationDuration: `${2 + (i % 5) * 0.4}s` }}
          />
        );
      })}
    </div>
  );
}
