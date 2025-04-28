export default function duration(time: number, withHour: boolean = false) {
    const seconds = Math.floor(time);
    return `${withHour ? `${Math.floor(seconds / 3600)}:` : ''}${`${Math.floor(seconds / 60) % 60}`.padStart(2, '0')}:${`${seconds % 60}`.padStart(2, '0')}`;
}
