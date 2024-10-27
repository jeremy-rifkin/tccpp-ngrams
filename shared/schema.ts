export type entry = { year_month: number; frequency: number };
export type query_result = Map<string, entry[]>;
export type query_response = query_result[];
export type encoded_query_result = [string, entry[]][];
export type encoded_query_response = {
    series: encoded_query_result[];
    time: number;
    error?: string;
};
