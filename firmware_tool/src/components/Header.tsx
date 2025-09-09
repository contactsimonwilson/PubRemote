import { Cpu } from 'lucide-react'
import { HeaderLinksSelector } from './HeaderLinksSelector'

const Header: React.FC = () => {

  return (
    <header className="bg-gray-900">
      <div className="container flex justify-between mx-auto px-4 py-4">
        <div className="flex items-center gap-3">
          <Cpu className="h-8 w-8 text-blue-500" />
          <div>
            <h1 className="text-xl font-bold text-white">Pubmote Firmware Tool</h1>
            <p className="text-sm text-gray-400">Diagnostic tool and firmware updater</p>
          </div>
        </div>
        <div className="flex items-center gap-3">
          <HeaderLinksSelector></HeaderLinksSelector>
        </div>
      </div>
    </header>
  );
}


export default Header;