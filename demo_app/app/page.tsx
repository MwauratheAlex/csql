'use client'

import { createOrder, createUser } from './actions';

export default function HomePage() {
    return (
        <main className="container mx-auto p-6 grow">
            <Header />
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
            <Terminal />
        </main>
    );
}

const Header = () => {
    return (
        <header className="flex gap-4 border-b border-slate-200 mb-6">
            <button id="tab-visual"
                className="pb-3 px-2 border-b-2 border-slate-900 font-semibold text-slate-900 transition-all">
                <i className="fa-solid fa-table-columns mr-2"></i>Visual CRUD & Join
            </button>
            <button id="tab-terminal"
                className="pb-3 px-2 border-b-2 border-transparent text-slate-500 hover:text-slate-700 transition-all">
                <i className="fa-solid fa-terminal mr-2"></i>SQL Terminal
            </button>
        </header>
    );
}

const Terminal = () => {
    return (
        <div id="view-terminal"
            className="hidden h-150 flex flex-col bg-slate-900 rounded-xl shadow-2xl overflow-hidden border border-slate-700 font-mono text-sm">
            <div className="bg-slate-800 px-4 py-2 flex items-center gap-2 border-b border-slate-700">
                <div className="flex gap-1.5">
                    <div className="w-3 h-3 rounded-full bg-red-500"></div>
                    <div className="w-3 h-3 rounded-full bg-yellow-500"></div>
                    <div className="w-3 h-3 rounded-full bg-green-500"></div>
                </div>
                <div className="text-slate-400 ml-3 text-xs">csql-cli — remote connection</div>
            </div>

            <div id="terminal-output" className="grow p-4 overflow-y-auto space-y-2 text-slate-300">
                <div className="text-slate-500">CSQL v1.0.0 (x86_64-linux-gnu)</div>
                <div className="text-slate-500">Type 'help' for commands. Enter SQL to execute.</div>
                <br />
            </div>

            <div className="bg-slate-800 p-3 flex items-center gap-2 border-t border-slate-700">
                <span className="text-emerald-500 font-bold">db&gt</span>
                <input type="text" id="terminal-input" autoComplete="off"
                    className="bg-transparent border-none outline-none text-white grow font-mono placeholder-slate-600"
                    placeholder="SELECT * FROM users;" />
            </div>
        </div>
    );
}

const UserTable = () => {
    return (
        <div className="bg-white rounded-xl shadow-sm border border-slate-200 overflow-hidden">
            <div className="px-6 py-4 border-b border-slate-100 flex justify-between items-center bg-slate-50">
                <h3 className="font-bold text-slate-700">Users Table</h3>
                <button className="text-slate-500 hover:text-blue-600 text-sm">
                    <i className="fa-solid fa-rotate-right mr-1"></i> Refresh
                </button>
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
                        <tr className="hover:bg-slate-50 transition-colors">
                            <td className="px-6 py-3 font-mono text-slate-600">101</td>
                            <td className="px-6 py-3 font-medium text-slate-900">alice_dev</td>
                            <td className="px-6 py-3 text-slate-500">alice@example.com</td>
                            <td className="px-6 py-3 text-right space-x-2">
                                <button className="text-blue-600 hover:text-blue-800" title="Update"><i
                                    className="fa-solid fa-pen"></i></button>
                                <button className="text-red-500 hover:text-red-700" title="Delete"><i
                                    className="fa-solid fa-trash"></i></button>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    )
}

const JoinDemo = () => {
    return (
        <div className="bg-indigo-50 rounded-xl border border-indigo-100 p-6">
            <div className="flex justify-between items-center mb-4">
                <h3 className="font-bold text-indigo-900 flex items-center gap-2">
                    <i className="fa-solid fa-diagram-project"></i> Relational Join Demo
                </h3>
                <span className="text-xs bg-indigo-200 text-indigo-800 px-2 py-1 rounded font-mono">
                    SELECT users.name, orders.item FROM users JOIN orders ON...
                </span>
            </div>
            <p className="text-sm text-indigo-700 mb-4">
                This section demonstrates the database engine's ability to merge two tables (Users + Orders)
                based on a common key.
            </p>
            <div className="bg-white rounded-lg border border-indigo-100 overflow-hidden">
                <table className="w-full text-left text-sm">
                    <thead className="bg-indigo-100/50 text-indigo-900 font-medium">
                        <tr>
                            <th className="px-6 py-2">User Name</th>
                            <th className="px-6 py-2">Ordered Item</th>
                            <th className="px-6 py-2">Order ID</th>
                        </tr>
                    </thead>
                    <tbody id="joinTableBody" className="divide-y divide-indigo-50">
                        <tr>
                            <td className="px-6 py-3 text-slate-500 italic" colSpan={3}>Click Refresh to run JOIN
                                query...
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>

    );
}

const RegisterUserForm = () => {
    return (
        <div className="bg-white p-6 rounded-xl shadow-sm border border-slate-200">
            <h2 className="text-lg font-bold mb-4 flex items-center gap-2">
                <i className="fa-solid fa-user-plus text-blue-600"></i> Register User
            </h2>
            <form id="userForm" className="space-y-3" action={createUser}>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">
                        ID (PrimaryKey)
                    </label>
                    <input type="number" name="userId"
                        className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                        placeholder="e.g. 101" required />
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Username</label>
                    <input type="text" name="userName"
                        className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                        placeholder="e.g. alice_dev" required />
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Email</label>
                    <input type="email" name="userEmail"
                        className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
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
            <h2 className="text-lg font-bold mb-4 flex items-center gap-2">
                <i className="fa-solid fa-cart-shopping text-purple-600"></i> New Order
            </h2>
            <form id="orderForm" className="space-y-3" action={createOrder}>
                <div className="grid grid-cols-2 gap-3">
                    <div>
                        <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Order
                            ID
                        </label>
                        <input type="number" name="orderId" className="w-full p-2 border border-slate-300 rounded"
                            placeholder="501" />
                    </div>
                    <div>
                        <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">User ID</label>
                        <input type="number" name="userId" className="w-full p-2 border border-slate-300 rounded"
                            placeholder="101" />
                    </div>
                </div>
                <div>
                    <label className="block text-xs font-semibold text-slate-500 uppercase mb-1">Item Name</label>
                    <input type="text" name="orderItem" className="w-full p-2 border border-slate-300 rounded"
                        placeholder="Keyboard" />
                </div>
                <button type="submit"
                    className="w-full bg-purple-600 hover:bg-purple-700 text-white font-medium py-2 rounded transition-colors shadow-sm">
                    CREATE (Foreign Key)
                </button>
            </form>
        </div>
    );
}
