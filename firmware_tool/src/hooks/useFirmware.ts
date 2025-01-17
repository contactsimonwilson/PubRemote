import { useEffect, useState } from 'react';
import { FirmwareVersion, ReleaseType } from '../types';
import sortBy from 'lodash/sortBy';
import uniqBy from 'lodash/uniqBy';

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
        
        let firmwareVersions: FirmwareVersion[] = releases.map(release => {
          const variants = release.assets
            .filter(asset => asset.name.endsWith('.zip'))
            .map(asset => {
              const variant = asset.name.replace('.zip', '');
              return {
                variant,
                zipUrl: asset.browser_download_url,
                date: release.published_at,
              };
            });

          let releaseType: ReleaseType = release.prerelease ? ReleaseType.Prerelease : ReleaseType.Release;
          if (release.prerelease && release.tag_name.toLowerCase().includes('nightly')) {
            releaseType = ReleaseType.Nightly;
          }

          return {
            version: release.tag_name,
            date: release.published_at,
            releaseType,
            variants,
          };
        }).filter(version => version.variants.length > 0);

        firmwareVersions = sortBy(firmwareVersions, v => {
          // Order by release type
          if (v.releaseType === ReleaseType.Release) {
            return 0;
          }

          if (v.releaseType === ReleaseType.Prerelease) {
            return 1;
          }

          return 2;
        });

        firmwareVersions = uniqBy(firmwareVersions, 'releaseType');


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