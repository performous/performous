export type StringDict = {
    [key: string]: string
}

export type ScreenQuery = {
    [key: string]: string | number | boolean
}

export type LocalStorageData = {
    screen?: string
    screenQuery?: ScreenQuery
}

export type Data = {
    [key: string]: Data|string|number|boolean|null
}|Array<JSON>

export type Dataable = Data|Song|SongQuery

export interface Song {
    Title: string
    Artist: string
    Edition: string
    Language: string
    Creator: string
    Duration: number
    HasError: boolean
    ProvidedBy: string
    Comment: string
    Path: string
    Position?: number
}

export interface State {
    screen: string
    screenQuery: ScreenQuery
    language: StringDict
    database: Song[]
    folders: string[]
    languages: string[]
    playlist: Song[]
    timeout: number,
    song: Song|number|null
}
