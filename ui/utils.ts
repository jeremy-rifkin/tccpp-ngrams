export function http_get(url: string, callback: (x: string | Error) => any) {
    const request = new XMLHttpRequest();
    request.onreadystatechange = function () {
        if (request.readyState == 4) {
            if (request.status == 200 || request.status == 500) {
                callback(request.responseText);
            } else {
                callback(new Error(`HTTP Error ${request.statusText}`));
            }
        }
    };
    request.open("GET", url, true);
    request.send(null);
}

// https://stackoverflow.com/questions/75988682/debounce-in-javascript
export function debounce(callback: (...args: any[]) => void, wait: number) {
    // eslint-disable-next-line @typescript-eslint/naming-convention
    let timeoutId: number | undefined;
    return (...args: any[]) => {
        window.clearTimeout(timeoutId);
        timeoutId = window.setTimeout(() => {
            callback(...args);
        }, wait);
    };
}

export function round_down(n: number, p: number) {
    return Math.floor(n * Math.pow(10, p)) / Math.pow(10, p);
}

export function round_down_exponential(n: number, digits: number) {
    if (isNaN(n)) {
        return "NaN";
    } else if (!isFinite(n)) {
        return n < 0 ? "-inf" : "+inf";
    } else if (n === 0) {
        return "0e+0";
    }
    const exponent = Math.floor(Math.log10(Math.abs(n)));
    const coefficient = round_down(n / Math.pow(10, exponent), digits);
    return `${coefficient}e${exponent >= 0 ? "+" : ""}${exponent}`;
}

export function maybe_slash(str: string) {
    return str.endsWith("/") ? str : str + "/";
}
