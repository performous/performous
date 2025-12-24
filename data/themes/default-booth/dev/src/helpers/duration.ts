export default function duration(time: number, withHour: boolean = false) {
    const timeSeconds = Math.floor(time);
    const hours = withHour ? `${Math.floor(timeSeconds / 3600)}:` : '';
    const minutes = `${Math.floor(timeSeconds / 60) % 60}`.padStart(2, '0');
    const seconds = `${timeSeconds % 60}`.padStart(2, '0');
    return `${hours}${minutes}:${seconds}`;
}
