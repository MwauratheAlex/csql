import { NextResponse, NextRequest } from 'next/server'
import * as net from 'net'

/** 
 * Queries the DB
 * @param {string} sql - raw sql string
 * @returns {Promise} query result
 */
function queryDB(sql: string): Promise<string> {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();
        const DB_HOST = '127.0.0.1';
        const DB_PORT = 9000;

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


/** 
 * Calls QueryDB with with passed sql string in request body
 * @param {NextRequest} req - request
 * @returns  json result from the query
 */
export async function POST(req: NextRequest) {
    try {
        const body = await req.json()
        const sql = body.sql;

        if (!sql) {
            return NextResponse.json({ error: "No SQL provided" }, { status: 400 });
        }

        const result = await queryDB(sql);

        return NextResponse.json({ result: result })

    } catch (err) {
        return NextResponse.json(
            { error: "Failed to connect to DB", details: err.message },
            { status: 500 }
        )
    }
}





















































