interface IStockData {
    id: number;
    name: string;
    currency: string;
    stockMarketSymbol: string;
    stockMarketName: string;
    isin: string;
    symbol1: string
    symbol2: string
    extRefId: string // the most important value - key to use for the search
}

interface IStockQuote {
    id: number;
    extRefId: string;
    stockMarketName: string;
    price: number;
    currency: string;
    high: number;
    low: number;
    ask: number;
    bid: number
    changeAbsolute: number;
    changeRelative: number;
    quoteTimestamp: Date;
    lastChangeTimestamp: Date;
}

interface IStock extends IStockData, IStockQuote {
}

interface IStockDataBackend {
    convertSearchResponse(responseText: string): Array<IStockData>
    convertQuoteResponse(responseText: string, symbol: string): IStockQuote
    search(key: string, callback: (returnCode: number, httpRequest: XMLHttpRequest) => void): void;
    store(stockData: IStockData): void;
    updateQuotes(stocks: Array<IStock>, stockFinisheCallback: (param1: IStockQuote, param2: IStock) => void, allStocksFinishedCallback: (count: number, failed: number) => void, timeoutInSeconds:number): void;
    convertSearchResponseToStockData(searchResponse: SearchResultStockData): StockData;
}
