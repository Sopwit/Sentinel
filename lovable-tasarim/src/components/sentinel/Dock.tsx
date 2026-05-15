import { Link, useRouterState } from "@tanstack/react-router";
import { Brain, MessageSquareText, Network, Orbit, Sparkles } from "lucide-react";
import { cn } from "@/lib/utils";

const NAV = [
  { to: "/", label: "Core", icon: Orbit },
  { to: "/chat", label: "Dialogue", icon: MessageSquareText },
  { to: "/memory", label: "Memory", icon: Brain },
  { to: "/agents", label: "Agents", icon: Network },
] as const;

export function Dock() {
  const { location } = useRouterState();
  return (
    <div className="fixed bottom-6 left-1/2 -translate-x-1/2 z-50">
      <div className="glass-strong relative flex items-center gap-1 rounded-2xl px-3 py-2 ring-soft shadow-[0_20px_60px_-20px_rgba(0,0,0,0.8)]">
        <div className="flex items-center justify-center h-10 w-10 rounded-xl">
          <Sparkles className="h-4 w-4 text-primary animate-breathe" />
        </div>
        <span className="h-8 w-px bg-white/10 mx-1" />
        {NAV.map(({ to, label, icon: Icon }) => {
          const active = location.pathname === to;
          return (
            <Link
              key={to}
              to={to}
              className={cn(
                "group relative flex flex-col items-center gap-0.5 rounded-xl px-4 py-2 transition-all hover:scale-105",
                active ? "text-primary" : "text-muted-foreground hover:text-foreground"
              )}
            >
              {active && <span className="absolute inset-0 rounded-xl glass ring-soft" />}
              <Icon className="relative h-[18px] w-[18px]" strokeWidth={1.4} />
              <span className="relative text-[8px] font-mono uppercase tracking-[0.2em]">{label}</span>
              {active && (
                <span className="absolute -bottom-2 left-1/2 -translate-x-1/2 h-1 w-1 rounded-full bg-primary shadow-[0_0_8px_currentColor]" />
              )}
            </Link>
          );
        })}
      </div>
    </div>
  );
}
