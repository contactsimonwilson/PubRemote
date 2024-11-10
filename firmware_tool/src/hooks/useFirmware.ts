import { useState, useEffect } from 'react';
import { FirmwareVersion } from '../types';

const GITHUB_REPO = 'contactsimonwilson/pubremote';
const GITHUB_API = 'https://api.github.com';

interface GitHubRelease {
  tag_name: string;
  published_at: string;
  assets: Array<{
    name: string;
    browser_download_url: string;
  }>;
}

export function useFirmware() {
  const [versions, setVersions] = useState<FirmwareVersion[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    async function loadFirmware() {
      try {
        const response = await fetch(`${GITHUB_API}/repos/${GITHUB_REPO}/releases`);
        if (!response.ok) {
          throw new Error('Failed to fetch releases');
        }
        
        const releases: GitHubRelease[] = await response.json();
        
        const firmwareVersions: FirmwareVersion[] = releases
          .map(release => {
            const bootloader = release.assets.find(asset => asset.name === 'bootloader.bin')?.browser_download_url;
            const partitionTable = release.assets.find(asset => asset.name === 'partitions.bin')?.browser_download_url;
            const application = release.assets.find(asset => asset.name === 'firmware.bin')?.browser_download_url;

            if (bootloader && partitionTable && application) {
              return {
                version: release.tag_name,
                date: release.published_at,
                bootloader,
                partitionTable,
                application
              };
            }
            return null;
          })
          .filter((version): version is FirmwareVersion => version !== null);

        setVersions(firmwareVersions);
        setLoading(false);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to load firmware versions');
        setLoading(false);
      }
    }

    loadFirmware();
  }, []);

  return { versions, loading, error };
}