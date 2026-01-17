'use server'

import { queryDB } from "./db";
import { revalidatePath } from "next/cache";

export interface User {
    id: number;
    username: string;
    email: string;
};

export async function createUser(formData: FormData) {
    const userId = formData.get("userId");
    const name = formData.get("userName");
    const email = formData.get("userEmail");

    const sql = `INSERT INTO users VALUES (${userId}, '${name}', '${email}');`;
    try {
        await queryDB(sql);
    } catch (err) {
        console.log("error: ", err);
    }

    revalidatePath("/");
}


export async function getUsers() { 
    const sql = `SELECT * FROM users;`;
    try {
        const result = await queryDB(sql);
        const user_rows = parseSQLResult(result);
        let users: User[] = [];

        for (let i = 0; i < user_rows.length; i++) {
            users.push({
                id: user_rows[i][0],
                username: user_rows[i][1],
                email: user_rows[i][2]
            })
        }

        return { success: true, message: users };
    } catch (err) {
        console.log("error: ", err);
        return { success: false, message: [] };
    }

}


export async function deleteUser(id: number) {
    const sql = `DELETE FROM users WHERE id=${id};`;
    try {
        await queryDB(sql);
        revalidatePath("/")
    } catch (err) {
        console.log("error: ", err);
    }
}

export async function updateUser(user: User) {
    const sql = `UPDATE users SET username='${user.username}', email='${user.email}' WHERE id=${user.id};`;
    try {
        await queryDB(sql);
        revalidatePath("/")
    } catch (err) {
        console.log("error: ", err);
    }
}

export async function createOrder(formData: FormData) {
    const orderId = formData.get('orderId');
    const userId = formData.get('userId');
    const item = formData.get('orderItem');

    try {
        const sql = `INSERT INTO orders VALUES (${orderId}, ${userId}, '${item}');`;
        await queryDB(sql);
        revalidatePath("/");
    } catch (err) {
        console.log("error: ", err);
    }
}

export async function getUsersOrders(): Promise<{
    success: boolean, 
    message: { 
        username: string; 
        order_item: string; 
        order_id: number; 
    }[]
}> {
    const sql = `SELECT users.username, orders.item, orders.id FROM users JOIN orders ON users.id=orders.user_id;`
    try {

        const result = await queryDB(sql);

        const rows = parseSQLResult(result);
        let users_orders: { 
            username: string; 
            order_item: string;
            order_id: number;
        }[] = [];

        for (let i = 0; i < rows.length; i++) {
            users_orders.push({
                username: rows[i][0],
                order_item: rows[i][1],
                order_id: rows[i][2],
            })
        }

        return { success: true, message: users_orders };
    } catch (err) {
        console.log("error: ", err);
        return { success: false, message: [] };
    }
}

export async function executeRawSQL(sql: string) {
    if (!sql || !sql.trim()) return "";

    try {
        const response = await queryDB(sql);
        return response;
    } catch (err: any) {
        return `Error: ${err.message}\n`
    }

}

const parseSQLResult = (raw: string): any[][] => {
    if (!raw) return [];

    const cleanRaw = raw.replace(/\0/g, '');

    return cleanRaw
        .split('\n')
        .map(line => line.trim())
        .filter(line => line.startsWith('(') && line.endsWith(')')) 
        .map(line => {
            try {
                const jsonArrayString = line.replace(/^\(/, '[').replace(/\)$/, ']');
                return JSON.parse(jsonArrayString);
            } catch (e) {
                console.error("Error parsing row:", line);
                return null;
            }
        })
        .filter(row => row !== null);
};































































































