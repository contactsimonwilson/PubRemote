import { useState, useEffect } from 'react';
import { FirmwareVersion } from '../types';

const GITHUB_REPO = 'contactsimonwilson/pubremote';
const GITHUB_API = 'https://api.github.com';

interface GitHubRelease {
  tag_name: string;
  published_at: string;
  prerelease: boolean;
  assets: Array<{
    name: string;
    browser_download_url: string;
    url: string;
  }>;
}

export function useFirmware() {
  const [versions, setVersions] = useState<FirmwareVersion[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    async function loadFirmware() {
      try {
        const response = await fetch(`${GITHUB_API}/repos/${GITHUB_REPO}/releases`, {
          headers: {
            'Accept': 'application/vnd.github.v3+json'
          }
        });
        
        if (!response.ok) {
          throw new Error(`Failed to fetch releases: ${response.statusText}`);
        }
        
        const releases: GitHubRelease[] = await response.json();
        
        const firmwareVersions: FirmwareVersion[] = releases.map(release => {
          const variants = release.assets
            .filter(asset => asset.name.endsWith('.zip'))
            .map(asset => {
              const variant = asset.name.replace(/^firmware_(.+)\.zip$/, '$1');
              return {
                variant,
                zipUrl: asset.browser_download_url,
                date: release.published_at,
              };
            });

          return {
            version: release.tag_name,
            date: release.published_at,
            prerelease: release.prerelease,
            variants,
          };
        }).filter(version => version.variants.length > 0);

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