import React from 'react';
import { Cpu } from 'lucide-react';

export function Header() {
  return (
    <header className="bg-gray-900">
      <div className="container mx-auto px-4 py-4">
        <div className="flex items-center gap-3">
          <Cpu className="h-8 w-8 text-blue-500" />
          <div>
            <h1 className="text-xl font-bold text-white">Pubmote Firmware Tool</h1>
            <p className="text-sm text-gray-400">Diagnostic tool and firmware updater</p>
          </div>
        </div>
      </div>
    </header>
  );
}