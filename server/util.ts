export function is_string(value: string | unknown): value is string {
    return typeof value === "string" || value instanceof String;
}
