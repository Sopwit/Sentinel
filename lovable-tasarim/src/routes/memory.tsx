import { createFileRoute } from "@tanstack/react-router";
import { Shell } from "@/components/sentinel/Shell";
import { Panel } from "@/components/sentinel/Panel";
import { useMemo, useState } from "react";
import { Archive, Bookmark, Brain, Clock, Database, Filter, Layers, Search, Sparkles, Tag } from "lucide-react";

export const Route = createFileRoute("/memory")({
  component: MemoryPage,
  head: () => ({ meta: [{ title: "Sentinel — Memory" }] }),
});

type Cluster = { id: string; label: string; count: number; hue: number };

const CLUSTERS: Cluster[] = [
  { id: "episodic", label: "Episodic", count: 184_220, hue: 210 },
  { id: "semantic", label: "Semantic", count: 982_104, hue: 200 },
  { id: "procedural", label: "Procedural", count: 64_930, hue: 250 },
  { id: "reflective", label: "Reflective", count: 41_502, hue: 180 },
  { id: "ambient", label: "Ambient", count: 912_011, hue: 220 },
];

type Shard = {
  id: string;
  title: string;
  cluster: string;
  weight: number;
  ts: string;
  tags: string[];
  excerpt: string;
};

const SHARDS: Shard[] = [
  { id: "M-2207", title: "Calm systems theorem · convergence proof", cluster: "semantic", weight: 0.94, ts: "21:42 · today", tags: ["theory", "calm"], excerpt: "Stable systems converge when feedback amplitude decays faster than perturbation entropy…" },
  { id: "M-2206", title: "Operator dialogue · evening session 27", cluster: "episodic", weight: 0.81, ts: "20:14 · today", tags: ["operator", "session"], excerpt: "Discussion drifted toward symmetry in cognition pathways. Resolved with bridge 1184." },
  { id: "M-2204", title: "Atlas perimeter sweep · semantic field", cluster: "procedural", weight: 0.66, ts: "18:02 · today", tags: ["agent:atlas", "scan"], excerpt: "No anomalies detected. Three weak resonances in cluster 7 logged for review." },
  { id: "M-2199", title: "Q4 horizon · synthesis brief draft", cluster: "reflective", weight: 0.88, ts: "yesterday", tags: ["q4", "brief"], excerpt: "Three candidate trajectories emerge. Path β preserves coherence under load." },
  { id: "M-2188", title: "Resonance pattern · 432 Hz ambient field", cluster: "ambient", weight: 0.42, ts: "tuesday", tags: ["signal", "harmonic"], excerpt: "Sustained harmonic in background channel. Correlates with operator focus periods." },
  { id: "M-2174", title: "Inference path β · attention re-weighting", cluster: "procedural", weight: 0.71, ts: "monday", tags: ["inference", "attention"], excerpt: "Re-balanced 14 active dialogues. Latency reduced 18%, coherence held at 0.97." },
];

function fmt(n: number) {
  return n >= 1_000_000 ? `${(n / 1_000_000).toFixed(2)}M` : n >= 1000 ? `${(n / 1000).toFixed(1)}k` : `${n}`;
}

function MemoryPage() {
  const [active, setActive] = useState<string>("all");
  const [selected, setSelected] = useState<Shard>(SHARDS[0]);

  const filtered = useMemo(
    () => (active === "all" ? SHARDS : SHARDS.filter((s) => s.cluster === active)),
    [active]
  );

  const total = CLUSTERS.reduce((a, c) => a + c.count, 0);

  return (
    <Shell title="Memory" subtitle="A living archive. Episodic, semantic, procedural — held in coherence.">
      {/* top stat strip */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4 mb-6">
        {[
          { label: "Total Shards", value: fmt(total), icon: Database, hint: "across 5 strata" },
          { label: "Coherence", value: "0.974", icon: Sparkles, hint: "above threshold" },
          { label: "Recall Latency", value: "12ms", icon: Clock, hint: "p95 · stable" },
          { label: "Index Age", value: "412d", icon: Archive, hint: "rebuilt 2h ago" },
        ].map((s) => (
          <Panel key={s.label} className="p-5">
            <div className="flex items-center justify-between text-muted-foreground">
              <s.icon className="h-3.5 w-3.5" strokeWidth={1.4} />
              <span className="text-[10px] font-mono uppercase tracking-widest">{s.hint}</span>
            </div>
            <div className="mt-5 text-2xl font-light tracking-tight">{s.value}</div>
            <div className="mt-1 text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground">{s.label}</div>
          </Panel>
        ))}
      </div>

      <div className="grid grid-cols-12 gap-6">
        {/* Left — strata */}
        <Panel className="col-span-12 lg:col-span-3 p-0" label="Strata">
          <div className="px-3 pb-4 pt-2 space-y-1">
            <button
              onClick={() => setActive("all")}
              className={`w-full flex items-center justify-between px-3 py-2.5 rounded-lg text-left transition ${
                active === "all" ? "glass ring-soft" : "hover:bg-white/[0.03]"
              }`}
            >
              <span className="flex items-center gap-2.5 text-sm">
                <Layers className="h-3.5 w-3.5 text-primary" strokeWidth={1.4} />
                All Memory
              </span>
              <span className="text-[10px] font-mono text-muted-foreground">{fmt(total)}</span>
            </button>
            {CLUSTERS.map((c) => (
              <button
                key={c.id}
                onClick={() => setActive(c.id)}
                className={`w-full flex items-center justify-between px-3 py-2.5 rounded-lg text-left transition ${
                  active === c.id ? "glass ring-soft" : "hover:bg-white/[0.03]"
                }`}
              >
                <span className="flex items-center gap-2.5 text-sm">
                  <span
                    className="h-2 w-2 rounded-sm"
                    style={{ background: `oklch(0.78 0.14 ${c.hue})`, boxShadow: `0 0 8px oklch(0.78 0.14 ${c.hue} / 0.6)` }}
                  />
                  {c.label}
                </span>
                <span className="text-[10px] font-mono text-muted-foreground">{fmt(c.count)}</span>
              </button>
            ))}
          </div>

          <div className="px-5 py-4 border-t border-white/5">
            <div className="text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground mb-3">Distribution</div>
            <div className="flex h-2 rounded-full overflow-hidden bg-white/[0.04]">
              {CLUSTERS.map((c) => (
                <div
                  key={c.id}
                  style={{
                    width: `${(c.count / total) * 100}%`,
                    background: `oklch(0.78 0.14 ${c.hue})`,
                  }}
                />
              ))}
            </div>
          </div>
        </Panel>

        {/* Middle — shards list */}
        <Panel
          className="col-span-12 lg:col-span-5 p-0"
          label={`Shards · ${active === "all" ? "all strata" : CLUSTERS.find(c => c.id === active)?.label}`}
          corner={
            <div className="flex items-center gap-2">
              <div className="flex items-center gap-1.5 glass rounded-full px-2.5 py-1">
                <Search className="h-3 w-3 text-muted-foreground" />
                <input
                  placeholder="query…"
                  className="bg-transparent outline-none text-[11px] w-32 placeholder:text-muted-foreground/50"
                />
              </div>
              <button className="glass rounded-full p-1.5 text-muted-foreground hover:text-primary transition">
                <Filter className="h-3 w-3" />
              </button>
            </div>
          }
        >
          <div className="px-2 pb-3 pt-1">
            {filtered.map((s) => {
              const cluster = CLUSTERS.find((c) => c.id === s.cluster)!;
              const active = selected.id === s.id;
              return (
                <button
                  key={s.id}
                  onClick={() => setSelected(s)}
                  className={`w-full text-left px-4 py-3.5 rounded-xl transition border ${
                    active ? "bg-white/[0.04] border-primary/20" : "border-transparent hover:bg-white/[0.02]"
                  }`}
                >
                  <div className="flex items-start justify-between gap-3">
                    <div className="flex items-center gap-2 min-w-0">
                      <span
                        className="h-1.5 w-1.5 rounded-full shrink-0"
                        style={{ background: `oklch(0.78 0.14 ${cluster.hue})`, boxShadow: `0 0 6px oklch(0.78 0.14 ${cluster.hue})` }}
                      />
                      <span className="text-[10px] font-mono text-muted-foreground tracking-widest">{s.id}</span>
                      <span className="text-[10px] font-mono text-muted-foreground/60 uppercase tracking-widest">· {cluster.label}</span>
                    </div>
                    <span className="text-[10px] font-mono text-muted-foreground/70 shrink-0">{s.ts}</span>
                  </div>
                  <div className="mt-1.5 text-sm text-foreground/95 truncate">{s.title}</div>
                  <div className="mt-2 flex items-center gap-3">
                    <div className="flex-1 h-[2px] bg-white/[0.04] rounded overflow-hidden">
                      <div
                        className="h-full"
                        style={{
                          width: `${s.weight * 100}%`,
                          background: `linear-gradient(to right, oklch(0.78 0.14 ${cluster.hue} / 0.5), oklch(0.86 0.12 ${cluster.hue}))`,
                        }}
                      />
                    </div>
                    <span className="text-[9px] font-mono text-muted-foreground tabular-nums">w {s.weight.toFixed(2)}</span>
                  </div>
                </button>
              );
            })}
          </div>
        </Panel>

        {/* Right — detail */}
        <div className="col-span-12 lg:col-span-4 space-y-6">
          <Panel className="p-0" label="Shard Detail" corner={
            <button className="text-muted-foreground hover:text-primary transition"><Bookmark className="h-3 w-3" /></button>
          }>
            <div className="px-5 pb-5 pt-2">
              <div className="flex items-center gap-2 text-[10px] font-mono text-muted-foreground tracking-widest">
                <span>{selected.id}</span>
                <span>·</span>
                <span className="uppercase">{CLUSTERS.find(c => c.id === selected.cluster)?.label}</span>
              </div>
              <h3 className="mt-2 text-base font-light leading-snug">{selected.title}</h3>
              <p className="mt-3 text-sm text-muted-foreground leading-relaxed">{selected.excerpt}</p>

              <div className="mt-5 grid grid-cols-3 gap-3 pt-4 border-t border-white/5">
                <Stat k="Weight" v={selected.weight.toFixed(2)} />
                <Stat k="Recalls" v="14" />
                <Stat k="Bridges" v="6" />
              </div>

              <div className="mt-5 flex flex-wrap gap-1.5">
                {selected.tags.map((t) => (
                  <span key={t} className="inline-flex items-center gap-1 rounded-full glass px-2.5 py-1 text-[10px] font-mono text-muted-foreground">
                    <Tag className="h-2.5 w-2.5" /> {t}
                  </span>
                ))}
              </div>
            </div>
          </Panel>

          <Panel className="p-0" label="Associative Field">
            <div className="px-5 pb-5 pt-2">
              <AssociativeField hue={CLUSTERS.find(c => c.id === selected.cluster)?.hue ?? 210} />
              <div className="mt-3 flex items-center justify-between text-[10px] font-mono text-muted-foreground">
                <span>6 bridges · 18 weak ties</span>
                <span className="flex items-center gap-1.5"><Brain className="h-3 w-3 text-primary" /> live</span>
              </div>
            </div>
          </Panel>
        </div>
      </div>
    </Shell>
  );
}

function Stat({ k, v }: { k: string; v: string }) {
  return (
    <div>
      <div className="text-lg font-light">{v}</div>
      <div className="text-[9px] font-mono uppercase tracking-[0.2em] text-muted-foreground">{k}</div>
    </div>
  );
}

function AssociativeField({ hue }: { hue: number }) {
  const center = { x: 140, y: 80 };
  const nodes = [
    { x: 30, y: 30, r: 4, label: "Symmetry" },
    { x: 250, y: 26, r: 5, label: "Coherence" },
    { x: 20, y: 130, r: 3, label: "Atlas" },
    { x: 260, y: 140, r: 4, label: "Field β" },
    { x: 90, y: 150, r: 3, label: "Bridge 1184" },
    { x: 200, y: 60, r: 4, label: "Q4" },
  ];
  const color = `oklch(0.82 0.13 ${hue})`;
  return (
    <svg viewBox="0 0 280 170" className="w-full h-44">
      <defs>
        <radialGradient id="af-glow" cx="50%" cy="50%">
          <stop offset="0%" stopColor={color} stopOpacity="0.25" />
          <stop offset="100%" stopColor={color} stopOpacity="0" />
        </radialGradient>
      </defs>
      <circle cx={center.x} cy={center.y} r="60" fill="url(#af-glow)" />
      {nodes.map((n, i) => (
        <line key={`l-${i}`} x1={center.x} y1={center.y} x2={n.x} y2={n.y} stroke={color} strokeOpacity="0.18" strokeWidth="0.6" />
      ))}
      {nodes.map((n, i) => (
        <g key={i}>
          <circle cx={n.x} cy={n.y} r={n.r} fill={color} opacity="0.85" />
          <text x={n.x + n.r + 5} y={n.y + 3} fill="oklch(0.85 0.01 220)" fontSize="8" fontFamily="ui-monospace, monospace" letterSpacing="0.05em">
            {n.label.toUpperCase()}
          </text>
        </g>
      ))}
      <circle cx={center.x} cy={center.y} r="6" fill={color} />
      <circle cx={center.x} cy={center.y} r="10" fill="none" stroke={color} strokeOpacity="0.5" />
    </svg>
  );
}
