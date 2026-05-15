import { Link, useRouterState } from "@tanstack/react-router";
import { Activity, Brain, MessageSquareText, Network, Orbit, Sparkles } from "lucide-react";
import { cn } from "@/lib/utils";

const NAV = [
  { to: "/", label: "Core", icon: Orbit },
  { to: "/chat", label: "Dialogue", icon: MessageSquareText },
  { to: "/memory", label: "Memory", icon: Brain },
  { to: "/agents", label: "Agents", icon: Network },
] as const;

export function Sidebar() {
  const { location } = useRouterState();
  return (
    <aside className="hidden lg:flex flex-col w-[88px] shrink-0 py-6 items-center gap-8 border-r border-white/5">
      <div className="relative h-10 w-10 rounded-xl glass-strong flex items-center justify-center halo">
        <Sparkles className="h-4 w-4 text-primary" />
        <span className="absolute -inset-1 rounded-xl border border-primary/20 animate-pulse-ring" />
      </div>

      <nav className="flex flex-col gap-2">
        {NAV.map(({ to, label, icon: Icon }) => {
          const active = location.pathname === to;
          return (
            <Link
              key={to}
              to={to}
              className={cn(
                "group relative flex flex-col items-center gap-1 rounded-xl px-3 py-3 transition",
                active ? "text-primary" : "text-muted-foreground hover:text-foreground"
              )}
            >
              {active && (
                <span className="absolute inset-0 rounded-xl glass ring-soft" />
              )}
              <Icon className="relative h-[18px] w-[18px]" strokeWidth={1.4} />
              <span className="relative text-[9px] font-mono uppercase tracking-[0.2em]">{label}</span>
            </Link>
          );
        })}
      </nav>

      <div className="mt-auto flex flex-col items-center gap-3">
        <div className="text-[9px] font-mono text-muted-foreground/70 tracking-[0.3em]">v2.7</div>
        <div className="h-8 w-8 rounded-full glass flex items-center justify-center">
          <Activity className="h-3 w-3 text-primary animate-breathe" />
        </div>
      </div>
    </aside>
  );
}
