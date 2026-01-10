import * as net from 'net'

const DB_HOST = '127.0.0.1';
const DB_PORT = 9000;

/** 
 * Queries the DB
 * @param {string} sql - raw sql string
 * @returns {Promise} query result
 */
export const queryDB = (sql: string): Promise<string> => {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();

        client.connect(DB_PORT, DB_HOST, () => {
            client.write(sql)
        })

        client.on('data', (data) => {
            resolve(data.toString())
            client.destroy()
        })

        client.on('error', (err) => {
            reject(err);
        })

        setTimeout(() => {
            client.destroy();
            reject(new Error("Database timeout"));
        }, 2000)
    })
}












