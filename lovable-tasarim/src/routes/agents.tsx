import { createFileRoute } from "@tanstack/react-router";
import { Shell } from "@/components/sentinel/Shell";
import { Panel } from "@/components/sentinel/Panel";

export const Route = createFileRoute("/agents")({
  component: AgentsPage,
  head: () => ({ meta: [{ title: "Sentinel — Agents" }] }),
});

const AGENTS = [
  { name: "Atlas", role: "Perimeter Cognition", state: "scanning", load: 62, color: "oklch(0.86 0.10 210)" },
  { name: "Orin", role: "Synthesis · Q4 Horizon", state: "thinking", load: 88, color: "oklch(0.78 0.14 200)" },
  { name: "Vela", role: "Ambient Listener", state: "listening", load: 24, color: "oklch(0.70 0.14 250)" },
  { name: "Kaze", role: "Memory Indexer", state: "indexing", load: 47, color: "oklch(0.85 0.06 180)" },
  { name: "Nyx", role: "Anomaly Watcher", state: "calm", load: 12, color: "oklch(0.74 0.10 230)" },
  { name: "Sol", role: "Forecast Modeller", state: "computing", load: 71, color: "oklch(0.82 0.12 215)" },
];

function AgentsPage() {
  return (
    <Shell title="Agents" subtitle="Six autonomous intelligences. Each operates within its own quiet rhythm.">
      <div className="grid grid-cols-12 gap-6">
        {AGENTS.map((a, i) => (
          <Panel key={a.name} className="col-span-12 md:col-span-6 xl:col-span-4 p-6" label={`Agent · ${String(i + 1).padStart(2, "0")}`}>
            <div className="px-5 pb-6 pt-2">
              <div className="flex items-start justify-between">
                <div>
                  <div className="text-2xl font-light tracking-tight">{a.name}</div>
                  <div className="text-xs text-muted-foreground mt-0.5">{a.role}</div>
                </div>
                <div className="text-[10px] font-mono uppercase tracking-[0.25em] text-primary">{a.state}</div>
              </div>

              {/* Orbit visualization */}
              <div className="relative my-6 h-44 flex items-center justify-center">
                <div className="absolute inset-0 flex items-center justify-center">
                  <div className="h-40 w-40 rounded-full border border-white/5 animate-orbit" style={{ animationDuration: `${20 + i * 4}s` }}>
                    <span className="absolute -top-1 left-1/2 -translate-x-1/2 h-1.5 w-1.5 rounded-full" style={{ background: a.color, boxShadow: `0 0 12px ${a.color}` }} />
                  </div>
                </div>
                <div className="absolute inset-0 flex items-center justify-center">
                  <div className="h-28 w-28 rounded-full border border-white/5 animate-orbit-rev" style={{ animationDuration: `${30 + i * 3}s` }}>
                    <span className="absolute top-1/2 -right-1 -translate-y-1/2 h-1 w-1 rounded-full" style={{ background: a.color, boxShadow: `0 0 8px ${a.color}` }} />
                  </div>
                </div>
                <div className="relative h-16 w-16 rounded-full glass-strong flex items-center justify-center halo animate-breathe">
                  <span className="h-3 w-3 rounded-full" style={{ background: a.color, boxShadow: `0 0 16px ${a.color}` }} />
                </div>
                <span className="absolute inset-0 rounded-full" />
              </div>

              <div className="space-y-3">
                <div className="flex items-center justify-between text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground">
                  <span>Load</span><span>{a.load}%</span>
                </div>
                <div className="h-[2px] w-full bg-white/5 rounded overflow-hidden">
                  <div className="h-full" style={{ width: `${a.load}%`, background: `linear-gradient(90deg, ${a.color}, oklch(0.95 0.04 210))`, boxShadow: `0 0 10px ${a.color}` }} />
                </div>
                <div className="grid grid-cols-3 gap-3 pt-2 text-[10px] font-mono">
                  <Stat label="Tasks" value={String(12 + i * 3)} />
                  <Stat label="Latency" value={`${8 + i * 2}ms`} />
                  <Stat label="Conf." value={`${(0.82 + i * 0.02).toFixed(2)}`} />
                </div>
              </div>
            </div>
          </Panel>
        ))}
      </div>
    </Shell>
  );
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div className="glass rounded-lg px-2.5 py-2">
      <div className="text-muted-foreground/70 uppercase tracking-widest text-[9px]">{label}</div>
      <div className="text-foreground text-[12px] mt-0.5">{value}</div>
    </div>
  );
}
