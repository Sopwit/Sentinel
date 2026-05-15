import { Search } from "lucide-react";
import { useEffect, useState } from "react";

export function TopBar({ title, subtitle }: { title: string; subtitle?: string }) {
  const [time, setTime] = useState("");
  useEffect(() => {
    const tick = () => setTime(new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }));
    tick();
    const id = setInterval(tick, 30_000);
    return () => clearInterval(id);
  }, []);
  return (
    <header className="flex items-center justify-between gap-6 px-8 pt-6">
      <div>
        <div className="flex items-center gap-3 text-[10px] font-mono uppercase tracking-[0.32em] text-muted-foreground">
          <span className="h-1.5 w-1.5 rounded-full bg-primary shadow-[0_0_8px_currentColor] animate-breathe" />
          Sentinel · Operating Layer
        </div>
        <h1 className="mt-2 text-3xl md:text-4xl font-light tracking-tight text-aurora">{title}</h1>
        {subtitle && <p className="mt-1 text-sm text-muted-foreground/80">{subtitle}</p>}
      </div>
      <div className="flex items-center gap-3">
        <div className="hidden md:flex items-center gap-2 glass rounded-full px-4 py-2 w-72">
          <Search className="h-3.5 w-3.5 text-muted-foreground" />
          <input
            placeholder="Ask Sentinel anything…"
            className="bg-transparent outline-none text-sm placeholder:text-muted-foreground/60 w-full"
          />
          <kbd className="text-[10px] font-mono text-muted-foreground/60">⌘K</kbd>
        </div>
        <div className="glass rounded-full px-4 py-2 text-[11px] font-mono text-muted-foreground tracking-widest">
          {time} · UTC
        </div>
      </div>
    </header>
  );
}
