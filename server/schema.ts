export type entry = { year_month: number; frequency: number };
export type query_result = Record<string, entry[]>;
export type query_response = query_result[];
