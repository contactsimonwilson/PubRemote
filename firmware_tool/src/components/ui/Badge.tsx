import { cn } from '../../utils/cn';

interface BadgeProps {
  children: React.ReactNode;
  variant?: 'default' | 'secondary' | 'destructive' | 'outline' | 'warning';
  className?: string;
}

export function Badge({ children, variant = 'default', className }: BadgeProps) {
  return (
    <span
      className={cn(
        'inline-flex items-center rounded-full px-2 py-0.5 text-xs font-medium',
        {
          'bg-blue-500/10 text-blue-500': variant === 'default',
          'bg-gray-500/10 text-gray-500': variant === 'secondary',
          'bg-red-500/10 text-red-500': variant === 'destructive',
          'bg-yellow-500/10 text-yellow-500': variant === 'warning',
          'border border-gray-200 dark:border-gray-800': variant === 'outline',
        },
        className
      )}
    >
      {children}
    </span>
  );
}