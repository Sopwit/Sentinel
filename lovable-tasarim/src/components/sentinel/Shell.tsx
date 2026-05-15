import type { ReactNode } from "react";
import { Atmosphere } from "./Atmosphere";
import { Dock } from "./Dock";
import { TopBar } from "./TopBar";

export function Shell({
  title,
  subtitle,
  children,
}: {
  title: string;
  subtitle?: string;
  children: ReactNode;
}) {
  return (
    <div className="min-h-screen">
      <Atmosphere />
      <main className="min-w-0">
        <TopBar title={title} subtitle={subtitle} />
        <div className="px-8 py-8 pb-32 animate-rise">{children}</div>
      </main>
      <Dock />
    </div>
  );
}
