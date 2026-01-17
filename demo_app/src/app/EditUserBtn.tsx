'use client'

import {
    FaPen,
} from "react-icons/fa6";
import { FaUserEdit } from "react-icons/fa";
import { IoIosClose } from "react-icons/io";

import { updateUser, User } from "../actions";
import { useState } from "react";

export const EditUserBtn = (props: { user: User }) => {
    const [is_modal_open, set_is_modal_open] = useState<Boolean>(false);
    const [show_success, set_show_success] = useState(false);


    return (
        <>
            {is_modal_open && (
                <div className="absolute top-0 bottom-0 right-0 left-0 z-9999 bg-black/10 flex items-center justify-center"
                    onClick={() => set_is_modal_open(false)}
                >
                    <div className="bg-white p-10 py-20 pt-12 rounded-xl shadow-sm border border-slate-200 w-lg space-y-4"
                        onClick={(e) => e.stopPropagation()}
                    >
                        <div className="flex justify-between items-center">
                            <h2 className="text-lg font-bold flex items-center gap-3 text-gray-900">
                                <FaUserEdit size={20} fill="#155dfc" />
                                Edit User
                            </h2>
                            <button className="text-red-500 hover:bg-black/10 border-none rounded-full 
                            shadow-none" title="Close" onClick={() => set_is_modal_open(false)}>
                                <IoIosClose size={28} />
                            </button>
                        </div>
                        <span className="text-xs bg-indigo-200 text-indigo-800 px-2 py-1 rounded font-mono w-full inline-block">
                            UPDATE users SET username='username', email='mail@user.com' WHERE id={props.user.id};
                        </span>

                        {show_success && (
                            <div className='text-black text-center p-2 bg-green-200 rounded-md transition-all
                            duration-1000 ease-in-out absolute w-sm top-24 tracking-wide'>
                                User updated successfully
                            </div>
                        )}
                        <form id="userForm" className="space-y-6 text-black" onSubmit={async (e) => {
                            e.preventDefault();
                            const formData = new FormData(e.currentTarget);
                            await updateUser({
                                id: props.user.id,
                                username: formData.get('userName') as string,
                                email: formData.get('userEmail') as string
                            });
                            set_show_success(true);
                            setTimeout(() => { set_show_success(false) }, 5000);
                        }}>
                            <div>
                                <label className="block text-xs font-semibold text-slate-500 uppercase mb-1 text-start">
                                    ID (PrimaryKey)
                                </label>
                                <input type="number" name="userId"
                                    className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                                    placeholder="e.g. 101" value={props.user.id} readOnly disabled />
                            </div>
                            <div>
                                <label className="block text-xs font-semibold text-slate-500 uppercase mb-1 text-start">Username</label>
                                <input type="text" name="userName"
                                    className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                                    placeholder="e.g. alice_dev" defaultValue={props.user.username} required />
                            </div>
                            <div>
                                <label className="block text-xs font-semibold text-slate-500 uppercase mb-1 text-start">Email</label>
                                <input type="email" name="userEmail"
                                    className="w-full p-2 border border-slate-300 rounded focus:ring-2 focus:ring-blue-500 focus:outline-none"
                                    placeholder="alice@example.com" required defaultValue={props.user.email} />
                            </div>
                            <button type="submit"
                                className="w-full bg-blue-600 hover:bg-blue-700 text-white font-medium py-2 rounded transition-colors shadow-sm mt-5">
                                UPDATE USER
                            </button>
                        </form>
                    </div>
                </div>
            )}
            <button className="text-blue-600 hover:text-blue-800 cursor-pointer py-1 px-4" 
                title="Update" 
                onClick={() => set_is_modal_open(true)}>
                <FaPen />
            </button>
        </>
    ); 
}
























































































































































































































































































































































































































































































































