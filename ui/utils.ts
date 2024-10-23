export function http_get(url: string, callback: (x: string) => any) {
    const request = new XMLHttpRequest();
    request.onreadystatechange = function () {
        if (request.readyState == 4 && (request.status == 200 || request.status == 500)) {
            callback(request.responseText);
        }
    };
    request.open("GET", url, true);
    request.send(null);
}

// https://stackoverflow.com/questions/75988682/debounce-in-javascript
export function debounce(callback: (...args: any[]) => void, wait: number) {
    let timeoutId: number | undefined;
    return (...args: any[]) => {
        window.clearTimeout(timeoutId);
        timeoutId = window.setTimeout(() => {
            callback(...args);
        }, wait);
    };
}
