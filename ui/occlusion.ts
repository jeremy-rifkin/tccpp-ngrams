// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-nocheck

import * as d3 from "d3";
import * as Plot from "@observablehq/plot";

// https://github.com/observablehq/plot/pull/1957/files
export function occlusionY(occlusionOptions = {}, options = {}) {
    // eslint-disable-next-line curly
    if (arguments.length === 1) [occlusionOptions, options] = mergeOptions(occlusionOptions);
    const { minDistance = 11 } = maybeDistance(occlusionOptions);
    return occlusion("y", "x", minDistance, options);
}

function maybeDistance(minDistance) {
    return typeof minDistance === "number" ? { minDistance } : minDistance;
}
function mergeOptions({ minDistance, ...options }) {
    return [{ minDistance }, options];
}

function occlusion(k, h, minDistance, options) {
    const sk = k[0]; // e.g., the scale for x1 is x
    if (typeof minDistance !== "number" || !(minDistance >= 0))
        // eslint-disable-next-line curly
        throw new Error(`unsupported minDistance ${minDistance}`);
    // eslint-disable-next-line curly
    if (minDistance === 0) return options;
    return Plot.initializer(options, function (data, facets, { [k]: channel }, { [sk]: s }) {
        const { value, scale } = channel ?? {};
        // eslint-disable-next-line curly
        if (value === undefined) throw new Error(`missing channel ${k}`);
        const K = value.slice();
        const H = Plot.valueof(data, options[h]);
        const bisect = d3.bisector(d => d.lo).left;

        for (const facet of facets) {
            for (const index of H ? d3.group(facet, i => H[i]).values() : [facet]) {
                const groups = [];
                for (const i of index) {
                    // eslint-disable-next-line curly
                    if (scale === sk) K[i] = s(K[i]);
                    let j = bisect(groups, K[i]);
                    groups.splice(j, 0, { lo: K[i], hi: K[i], items: [i] });

                    // Merge overlapping groups.
                    while (
                        groups[j + 1]?.lo < groups[j].hi + minDistance ||
                        // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
                        (groups[j - 1]?.hi > groups[j].lo - minDistance && (--j, true))
                    ) {
                        const items = groups[j].items.concat(groups[j + 1].items);
                        const mid =
                            (Math.min(groups[j].lo, groups[j + 1].lo) + Math.max(groups[j].hi, groups[j + 1].hi)) / 2;
                        const w = (minDistance * (items.length - 1)) / 2;
                        groups.splice(j, 2, { lo: mid - w, hi: mid + w, items });
                    }
                }

                // Reposition elements within each group.
                for (const { lo, hi, items } of groups) {
                    if (items.length > 1) {
                        const dist = (hi - lo) / (items.length - 1);
                        items.sort((i, j) => K[i] - K[j]);
                        let p = lo;
                        // eslint-disable-next-line curly, @typescript-eslint/no-unused-expressions
                        for (const i of items) (K[i] = p), (p += dist);
                    }
                }
            }
        }

        return { data, facets, channels: { [k]: { value: K, source: null } } };
    });
}
