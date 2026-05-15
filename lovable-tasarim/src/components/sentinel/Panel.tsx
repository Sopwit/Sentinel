import { cn } from "@/lib/utils";
import type { ReactNode } from "react";

export function Panel({
  children,
  className,
  label,
  corner,
}: {
  children: ReactNode;
  className?: string;
  label?: string;
  corner?: ReactNode;
}) {
  return (
    <div className={cn("glass relative overflow-hidden rounded-2xl", className)}>
      {/* corner brackets */}
      <CornerBrackets />
      {label && (
        <div className="flex items-center justify-between px-5 pt-4">
          <div className="flex items-center gap-2 text-[10px] font-mono uppercase tracking-[0.25em] text-muted-foreground">
            <span className="h-1 w-1 rounded-full bg-primary shadow-[0_0_6px_currentColor]" />
            {label}
          </div>
          {corner}
        </div>
      )}
      <div className="relative">{children}</div>
    </div>
  );
}

function CornerBrackets() {
  const c = "absolute h-3 w-3 border-primary/50";
  return (
    <>
      <span className={`${c} top-2 left-2 border-t border-l rounded-tl`} />
      <span className={`${c} top-2 right-2 border-t border-r rounded-tr`} />
      <span className={`${c} bottom-2 left-2 border-b border-l rounded-bl`} />
      <span className={`${c} bottom-2 right-2 border-b border-r rounded-br`} />
    </>
  );
}
