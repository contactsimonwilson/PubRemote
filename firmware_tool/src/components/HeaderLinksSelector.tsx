import { Info } from 'lucide-react'
import { Dropdown } from './ui/Dropdown'

export function HeaderLinksSelector() {
  const handleLinkSelect = (url: string) => {
    window.open(url, '_blank');
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <Dropdown
        options={[
          {
            value: "https://github.com/contactsimonwilson/PubRemote",
            tooltip: `PubRemote GitHub repository`,
            label: (
              <div className="flex items-center justify-between w-full">
                <div className="flex-1 truncate">
                  <span className="mr-2">PubRemote GitHub repository</span>
                </div>
              </div>
            )
          },
          {
            value: "https://github.com/Relys/vesc_pkg/tree/float-accessories",
            tooltip: `FloatAccessories GitHub repository`,
            label: (
              <div className="flex items-center justify-between w-full">
                <div className="flex-1 truncate">
                  <span className="mr-2">FloatAccessories GitHub repository</span>
                </div>
              </div>
            )
          },
          {
            value: "https://discord.gg/7hTnbgPfKt",
            tooltip: `Pubmote discussion channel in PubWheel Discord`,
            label: (
              <div className="flex items-center justify-between w-full">
                <div className="flex-1 truncate">
                  <span className="mr-2">Pubmote on PubWheel Discord</span>
                </div>
              </div>
            )
          }
        ]}
        value=""
        onChange={(value) => handleLinkSelect(value as string)}
        icon={<Info className="h-4 w-4" />}
        label="Useful Links"
        width="fixed"
        dropdownWidth={400}
        />
      </div>
    </div>
  );
}