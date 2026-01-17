'use server'
import { createOrder, createUser, getUsers, getUsersOrders, User } from '../actions';
import {
    FaTableColumns,
    FaTerminal,
    FaUserPlus,
    FaCartShopping
} from "react-icons/fa6";
import { db_init, db_init_test } from '../db';
import Link from 'next/link';
import { DeleteUserBtn } from "./DeleteUserBtn"
import { EditUserBtn } from "./EditUserBtn"

export default async function HomePage() {
    // await db_init_test(); // stress tests our db logic
    await db_init(); //normal init 
    return (
        <main className="mx-auto grow border w-full overflow-y-scroll">
            <div className="container  mx-auto py-6">
                <Header />
            </div>
            <div 
                className="flex flex-col
                shadow-black container mx-auto">
                <div id="view-visual" className="grid grid-cols-1 lg:grid-cols-3 gap-8 fade-in">
                    <div className="space-y-6">
                        <RegisterUserForm />
                        <NewOrderForm />
                    </div>
                    <div className="lg:col-span-2 space-y-6">
                        <UserTable />
                        <JoinDemo />
                    </div>
                </div>
            </div>
        </main>
    );
}

const Header = () => {
    return (
        <header className="flex gap-4 border-b border-slate-200 mb-6">
            <Link id="tab-visual" href="/"
                className="pb-3 px-2 border-b-2 border-slate-900 font-semibold 
                text-slate-900 transition-all flex items-center gap-2">
                <FaTableColumns size={18} />
            </Link>
            <Link href="/terminal" id="tab-terminal"
                className="pb-3 px-2 border-b-2 border-transparent hover:text-slate-700 
                text-slate-500 transition-all flex items-center"> 
                <FaTerminal size={18} /> SQL Terminal
            </Link>
        </header>
    );
}

const UserTable = async () => {
    const result = await getUsers();
    let users: User[] = [];
    if (result.success) {
        users = result.message;
    }

    return (
        <div className="bg-white rounded-xl shadow-sm border border-slate-200 overflow-hidden">
            <div className="px-6 py-4 border-b border-slate-100 flex justify-between items-center bg-slate-50">
                <h3 className="font-bold text-slate-700">Users Table</h3>
                <span className="text-xs bg-indigo-200 text-indigo-800 px-2 py-1 rounded font-mono text-nowrap">
                    SELECT * FROM users;
                </span>
            </div>
            <div className="overflow-x-auto">
                <table className="w-full text-left text-sm">
                    <thead className="bg-slate-50 text-slate-500 font-medium border-b border-slate-200">
                        <tr>
                            <th className="px-6 py-3">ID (PK)</th>
                            <th className="px-6 py-3">Username</th>
                            <th className="px-6 py-3">Email</th>
                            <th className="px-6 py-3 text-right">Actions</th>
                        </tr>
                    </thead>
                    <tbody id="usersTableBody" className="divide-y divide-slate-100">
                        {users.map((user) => (

                            <tr className="hover:bg-slate-50 transition-colors" key={user.id}>
                                <td className="px-6 py-3 font-mono text-slate-600">{user.id}</td>
                                <td className="px-6 py-3 font-medium text-slate-900">{user.username}</td>
                                <td className="px-6 py-3 text-slate-500">{user.email}</td>
                                <td className="px-6 py-3 text-right space-x-2">
                                    <EditUserBtn user={user} />
                                    <DeleteUserBtn userId={user.id} />
                                </td>
                            </tr>
                        ))}
                        {users.length === 0 && (
                            <tr>
                                <td colSpan={4} className="text-center py-8 text-slate-500 italic">
                                    No users found
                                </td>
                            </tr>
                        )}
                    </tbody>
                </table>
            </div>
        </div>
    )
}

const JoinDemo = async () => {
    const users_orders = await getUsersOrders();

    return (
        <div className="bg-white rounded-xl shadow-sm border border-slate-200 overflow-hidden">
            <div className="px-6 py-4 border-b border-slate-100 flex justify-between items-center bg-slate-50">
                <h3 className="font-bold text-slate-700 flex items-center gap-2">
                    Join
                </h3>
                <span className="text-xs bg-indigo-200 text-indigo-800 px-2 py-1 rounded font-mono text-nowrap">
                    SELECT users.username, orders.item, orders.id FROM users JOIN orders ON users.id=orders.user_id;
                </span>
            </div>

            <div className="overflow-x-auto">
                <table className="w-full text-left text-sm">
                    <thead className="bg-slate-50 text-slate-500 font-medium border-b border-slate-200">
                        <tr>
                            <th className="px-6 py-3">User Name</th>
                            <th className="px-6 py-3">Ordered Item</th>
                            <th className="px-6 py-3">Order ID</th>
                        </tr>
                    </thead>
                    <tbody id="joinTableBody" className="divide-y divide-slate-100">
                        {users_orders.message.map((user_order) => (
                            <tr className="hover:bg-slate-50 transition-colors" key={user_order.order_id}>
                                <td className="px-6 py-3 font-medium text-slate-900">{user_order.username}</td>
                                <td className="px-6 py-3 text-slate-500">{user_order.order_item}</td>
                                <td className="px-6 py-3 font-mono text-slate-600">{user_order.order_id}</td>
                            </tr>
                        ))}

                        {users_orders.message.length === 0 && (
                            <tr>
                                <td colSpan={4} className="text-center py-8 text-slate-500 italic">
                                    No user with orders found.
                                </td>
                            </tr>
                        )}
                    </tbody>
                </table>
            </div>
        </div>
    );
}

const RegisterUserForm = () => {
    return (
        <div className="bg-white p-6 rounded-xl shadow-sm border border-slate-200">
            <h2 className="text-lg font-bold mb-4 flex items-center gap-3 text-gray-900">
                <FaUserPlus size={20} fill="#155dfc" />
                Register User
            </h2>
            <form id="userForm" className="space-y-3" action={createUser}>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">
                        ID (PrimaryKey)
                    </label>
                    <input type="number" name="userId"
                        className="w-full text-black placeholder:accent-gray-500 p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 
                        focus:outline-none focus:bg-white"
                        placeholder="e.g. 101" required />
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Username</label>
                    <input type="text" name="userName"
                        className="w-full text-black placeholder:accent-gray-500 p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none focus:bg-white input bg-white"
                        placeholder="e.g. alice_dev" required />
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Email</label>
                    <input type="email" name="userEmail"
                        className="w-full p-2 border border-slate-300 text-black placeholder:accent-gray-500
                        rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                        placeholder="alice@example.com" required />
                </div>
                <button type="submit"
                    className="w-full bg-blue-600 hover:bg-blue-700 text-white font-medium py-2 rounded transition-colors shadow-sm">
                    CREATE USER
                </button>
            </form>
        </div>
    );

}

const NewOrderForm = () => {
    return (
        <div className="bg-white p-6 rounded-xl shadow-sm border border-slate-200">
            <h2 className="text-lg font-bold mb-4 flex items-center gap-3 text-gray-900">
                <FaCartShopping size={20} fill="#9810fa" />
                New Order
            </h2>
            <form id="orderForm" className="space-y-3" action={createOrder}>
                <div className="grid grid-cols-2 gap-3">
                    <div>
                        <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Order
                            ID
                        </label>
                        <input type="number" name="orderId" className="w-full text-black placeholder:accent-gray-500 p-2 border border-slate-300 rounded"
                            placeholder="501" />
                    </div>
                    <div>
                        <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">User ID</label>
                        <input type="number" name="userId" className="w-full p-2 border border-slate-300 rounded text-black placeholder:accent-gray-500"
                            placeholder="101" />
                    </div>
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Item Name</label>
                    <input type="text" name="orderItem" className="w-full p-2 border border-slate-300 rounded text-black placeholder:accent-gray-500"
                        placeholder="Keyboard" />
                </div>
                <button type="submit"
                    className="w-full bg-blue-600  hover:bg-blue-800 text-white font-medium py-2 rounded transition-colors shadow-sm">
                    CREATE (Foreign Key)
                </button>
            </form>
        </div>
    );
}






























































































































































































































































