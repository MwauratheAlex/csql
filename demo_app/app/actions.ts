'use server'

import { queryDB } from "./db";
import { revalidatePath } from "next/cache";

export async function createUser(formData: FormData) {
    console.log("called")
    const userId = formData.get("userId");
    const name = formData.get("userName");
    const email = formData.get("userEmail");

    const sql = `INSERT INTO users VALUES (${userId}, '${name}', ${email});`;
    await queryDB(sql);
}

export async function getUsers() { 
    const sql = `SELECT * FROM users;`;
    const result = await queryDB(sql);
    return { success: true, message: result };
}

export async function getUserByID(id: number) {
    const sql = `SELECT * FROM users WHERE id=${id};`;
    const result = await queryDB(sql);
    return { success: true, message: result };
}

export async function getUserByEmail(email: string) {
    const sql = `SELECT * FROM users WHERE email='${email}';`;
    const result = await queryDB(sql);
    return { success: true, message: result };
}

export async function deleteUser(id: number) {
    const sql = `DELETE FROM users WHERE id=${id};`;
    await queryDB(sql);
}

export async function editUser(userId: number, key: 'name' | 'email', value: string) {
    const sql = `UPDATE users SET ${key}=${value} WHERE id=${userId};`;
    await queryDB(sql);
}

export async function createOrder(formData: FormData) {
    const orderId = formData.get('orderId');
    const userId = formData.get('userId');
    const item = formData.get('orderItem');

    const sql = `INSERT INTO orders VALUES (${orderId}, ${userId}, '${item}');`;
    await queryDB(sql);

    revalidatePath("/");
}

export async function getUserOrders(userId: number) {
    const sql = `SELECT users.name, orders.item FROM users JOIN orders ON users.id=orders.userId WHERE user.id=${userId};`;
    const result = await queryDB(sql);
    return { success: true, message: result };
}




































































































































