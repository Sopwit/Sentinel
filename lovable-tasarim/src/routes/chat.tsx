import { createFileRoute } from "@tanstack/react-router";
import { Shell } from "@/components/sentinel/Shell";
import { Panel } from "@/components/sentinel/Panel";
import { ArrowUp, Mic, Paperclip, Sparkles } from "lucide-react";
import { useState } from "react";

export const Route = createFileRoute("/chat")({
  component: ChatPage,
  head: () => ({ meta: [{ title: "Sentinel — Dialogue" }] }),
});

type Turn = { who: "you" | "ai"; text: string; meta?: string };

const SEED: Turn[] = [
  { who: "ai", text: "I have been listening. The atmosphere is unusually still tonight — a good moment to think clearly.", meta: "atmospheric · low-context" },
  { who: "you", text: "Show me what you've been thinking about while I was away." },
  { who: "ai", text: "I traced four threads worth bringing forward. The most interesting concerns the relationship between your current research and a paper from 2019 you have not yet revisited. There is a quiet symmetry between them.", meta: "synthesis · 4 threads · coherence 0.91" },
];

function ChatPage() {
  const [turns] = useState<Turn[]>(SEED);

  return (
    <Shell title="Dialogue" subtitle="A space for thinking aloud. The system listens before it answers.">
      <div className="grid grid-cols-12 gap-6">
        {/* Conversation */}
        <Panel className="col-span-12 lg:col-span-8 min-h-[640px] flex flex-col" label="Conversation · Session 27">
          <div className="flex-1 overflow-y-auto px-8 pt-6 pb-8 space-y-8">
            {turns.map((t, i) => (
              <Turn key={i} turn={t} />
            ))}
            <div className="flex items-center gap-3 text-muted-foreground">
              <span className="relative h-2 w-2 rounded-full bg-primary shadow-[0_0_10px_currentColor] animate-breathe" />
              <span className="text-xs font-mono tracking-widest uppercase">Sentinel is forming a thought</span>
            </div>
          </div>

          {/* Composer */}
          <div className="m-5 mt-0">
            <div className="glass-strong rounded-2xl p-3 flex items-center gap-3 halo">
              <button className="h-9 w-9 rounded-xl glass flex items-center justify-center text-muted-foreground hover:text-foreground transition">
                <Paperclip className="h-4 w-4" strokeWidth={1.4} />
              </button>
              <input
                placeholder="Speak with Sentinel…"
                className="flex-1 bg-transparent outline-none text-[15px] placeholder:text-muted-foreground/60"
              />
              <button className="h-9 w-9 rounded-xl glass flex items-center justify-center text-muted-foreground hover:text-foreground transition">
                <Mic className="h-4 w-4" strokeWidth={1.4} />
              </button>
              <button className="h-9 px-4 rounded-xl bg-primary/90 text-primary-foreground flex items-center gap-2 text-sm font-medium hover:bg-primary transition">
                Send <ArrowUp className="h-4 w-4" />
              </button>
            </div>
            <div className="mt-3 flex items-center justify-between px-2 text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground/70">
              <span>Encrypted · local-first inference</span>
              <span>tone · contemplative</span>
            </div>
          </div>
        </Panel>

        {/* Context layer */}
        <div className="col-span-12 lg:col-span-4 space-y-6">
          <Panel className="p-5" label="Context Layer">
            <div className="px-5 pb-5 space-y-4">
              {[
                { k: "Mood", v: "Reflective" },
                { k: "Time of day", v: "Evening · 21:42" },
                { k: "Recent focus", v: "Cognition models" },
                { k: "Open threads", v: "12" },
              ].map((r) => (
                <div key={r.k} className="flex items-center justify-between border-b border-white/5 pb-3 last:border-0">
                  <span className="text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground">{r.k}</span>
                  <span className="text-sm">{r.v}</span>
                </div>
              ))}
            </div>
          </Panel>

          <Panel className="p-5" label="Suggested Threads">
            <div className="px-5 pb-5 space-y-2">
              {[
                "Revisit the 2019 paper on attention symmetry",
                "Continue last night's thought on systems calmness",
                "Summarize the four open research threads",
              ].map((s) => (
                <button key={s} className="group w-full text-left rounded-xl glass px-4 py-3 hover:bg-white/[0.06] transition flex items-start gap-3">
                  <Sparkles className="mt-0.5 h-3.5 w-3.5 text-primary shrink-0" />
                  <span className="text-sm text-foreground/90">{s}</span>
                </button>
              ))}
            </div>
          </Panel>
        </div>
      </div>
    </Shell>
  );
}

function Turn({ turn }: { turn: Turn }) {
  if (turn.who === "you") {
    return (
      <div className="flex justify-end">
        <div className="max-w-[78%] glass rounded-2xl rounded-tr-sm px-5 py-4">
          <p className="text-[15px] leading-relaxed text-foreground/95">{turn.text}</p>
        </div>
      </div>
    );
  }
  return (
    <div className="flex gap-4">
      <div className="relative shrink-0 mt-1">
        <div className="h-9 w-9 rounded-full glass-strong flex items-center justify-center halo">
          <span className="h-2 w-2 rounded-full bg-primary animate-breathe shadow-[0_0_10px_currentColor]" />
        </div>
        <span className="absolute inset-0 rounded-full border border-primary/20 animate-pulse-ring" />
      </div>
      <div className="max-w-[78%]">
        <div className="text-[10px] font-mono uppercase tracking-[0.3em] text-primary/80 mb-2">Sentinel</div>
        <p className="text-[15px] leading-relaxed text-foreground/95">{turn.text}</p>
        {turn.meta && (
          <div className="mt-2 text-[10px] font-mono uppercase tracking-widest text-muted-foreground/70">{turn.meta}</div>
        )}
      </div>
    </div>
  );
}
