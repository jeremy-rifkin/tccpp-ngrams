export const first_bucket: [number, number] = [2017, 6]; // july 2017
export const last_bucket: [number, number] = [2024, 8]; // september 2024

export function maybe_slash(str: string) {
    return str.endsWith("/") ? str : str + "/";
}
