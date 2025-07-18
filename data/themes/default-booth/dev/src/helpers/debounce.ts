export default function debounce(callback: Function, timeout: number = 300): Function {
    let timeoutId: number;
    return (...args: any) => {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => callback(...args), timeout);
    }
}
