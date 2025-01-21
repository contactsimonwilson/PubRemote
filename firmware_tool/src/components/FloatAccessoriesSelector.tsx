import { Tag } from 'lucide-react';
import { FirmwareVersion, ReleaseType } from '../types';
import { Badge } from './ui/Badge';
import { Dropdown } from './ui/Dropdown';

export function FloatAccessoriesSelector() {
  const versions = [
    {
      version: "v2.4.0",
      date: "1/20/2025",
      variants: [
        {
          zipUrl: "/float_accessories/float_accessories-2.4.vescpkg.zip",
          date: "1/20/2025",
          variant: "float_accessories-2.4.vescpkg"
        }
      ],
      releaseType: ReleaseType.Release
    }
  ] as FirmwareVersion[];

  const versionOptions = versions.flatMap(version =>
    version.variants.map(variant => ({
      value: variant.zipUrl,
      tooltip: `Version ${version.version} (${variant.variant}) - ${new Date(version.date).toLocaleDateString()}`,
      label: (
        <div className="flex items-center justify-between w-full">
          <div className="flex-1 truncate">
            <span className="mr-2">{version.version}</span>
            <span className="text-gray-400">{variant.variant}</span>
            <span className="ml-2 text-gray-400 text-xs">
              {new Date(version.date).toLocaleDateString()}
            </span>
          </div>
          {version.releaseType === ReleaseType.Release && (
            <Badge variant="default" className="ml-2 flex-shrink-0">
              Stable
            </Badge>
          )}
          {version.releaseType === ReleaseType.Prerelease && (
            <Badge variant="warning" className="ml-2 flex-shrink-0">
              Prerelease
            </Badge>
          )}
          {version.releaseType === ReleaseType.Nightly && (
            <Badge variant="destructive" className="ml-2 flex-shrink-0">
              Nightly
            </Badge>
          )}
        </div>
      ),
    }))
  );

  const handleVersionSelect = (url: string) => {
    window.open(url, '_blank');
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-semibold">Float Accessories Package</h2>
          <Dropdown
            options={versionOptions}
            value=""
            onChange={(value) => handleVersionSelect(value as string)}
            label="Download Float Accessories"
            icon={<Tag className="h-4 w-4" />}
            disabled={versions.length === 0}
            width="fixed"
            dropdownWidth={400}
          />
      </div>
    </div>
  );
}