'use server'
import * as net from 'net'

const DB_HOST = '127.0.0.1';
const DB_PORT = 9000;

export const queryDB = async (sql: string): Promise<string> => {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();
        let responseBuffer = "";

        client.connect(DB_PORT, DB_HOST, () => {
            client.write(sql);
        });

        client.on('data', (chunk) => {
            const chunkStr = chunk.toString();
            responseBuffer += chunkStr;

            if (chunkStr.includes('\0')) {
                const finalData = responseBuffer.replace(/\0/g, '');
                resolve(finalData);
                client.destroy(); 
            }
        });

        client.on('error', (err) => {
            reject(err);
        });

        setTimeout(() => {
            if (!client.destroyed) {
                client.destroy();
                reject(new Error("Database timeout"));
            }
        }, 5000);
    });
};

export const db_init = async () => {
    try {
        const res = await queryDB("CREATE TABLE users (id int PRIMARY KEY, username text, email text);");
        if (res.includes("Error")) return;

        await queryDB("CREATE TABLE orders (id int PRIMARY KEY, user_id int, item text);");
        await queryDB("CREATE INDEX idx_orders_user ON orders(user_id);");
        await queryDB("INSERT INTO users VALUES (101, 'Mwaura Mbugua', 'mwaurambugua12@gmail.com');");
        await queryDB("INSERT INTO orders VALUES (501, 101, 'Keyboard');");
        console.log("Database initialized successfully!");
    } catch (err) { 
        console.log("Error: ", err)
    }
}

export const db_init_test = async () => {
    console.log("Starting Database Stress Test...");
    const startTime = Date.now();

    try {
        // Setup Schema
        console.log("- Creating Schema...");
        await queryDB("CREATE TABLE users (id int PRIMARY KEY, username text, email text);");
        await queryDB("CREATE TABLE orders (id int PRIMARY KEY, user_id int, item text);");
        await queryDB("CREATE INDEX idx_orders_user ON orders(user_id);");

        // Bulk Insert (Users)
        console.log("-> Bulk Inserting 500 Users...");
        for (let i = 1; i <= 500; i++) {
            const sql = `INSERT INTO users VALUES (${i}, 'user_${i}', 'user_${i}@stress-test.com');`;
            const res = await queryDB(sql);
            if (res.includes("Error")) console.error(`Failed to insert user ${i}: ${res}`);
        }

        // Bulk Insert (Orders)
        console.log("-> Bulk Inserting 500 Orders...");
        for (let i = 1; i <= 500; i++) {
            const item = i % 2 === 0 ? "Keyboard" : "Monitor";
            const sql = `INSERT INTO orders VALUES (${i + 5000}, ${i}, '${item}');`; await queryDB(sql);
        }

        // Long String (Buffer Overflow Test)
        console.log("-> Testing Massive String Insert...");
        const longString = "A".repeat(1000);
        await queryDB(`INSERT INTO users VALUES (9999, 'massive_user', '${longString}');`);

        // Duplicate Key
        console.log("-> Testing Duplicate Key Constraint...");
        const dupRes = await queryDB("INSERT INTO users VALUES (1, 'duplicate_user', 'fail@test.com');");
        if (dupRes.includes("Error: Duplicate key")) {
            console.log("   Duplicate key correctly rejected.");
        } else {
            console.error(" FAILED: Duplicate key was accepted!", dupRes);
        }

        // Stress Query: Full Table Scan + Join
        console.log("-> Running Heavy Join (Users + Orders)...");
        const joinRes = await queryDB("SELECT users.username, orders.item FROM users JOIN orders ON users.id=orders.user_id;");

        const rowCount = joinRes.split('\n').filter(line => line.startsWith('(')).length;
        console.log(`   Join returned ${rowCount} rows.`);

        const duration = Date.now() - startTime;
        console.log(`\nStress Test Complete in ${duration}ms! Database is stable.`);

    } catch (err) { 
        console.log("\nDATABASE CRASHED OR TIMED OUT DURING STRESS TEST. Server started?");
        console.log("Error: ", err);
    }
}
