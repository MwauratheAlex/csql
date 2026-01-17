'use client'

import {
    FaTrash,
} from "react-icons/fa6";
import { deleteUser } from "@/actions";

export const DeleteUserBtn = (props: { userId: number }) => {
    const handleClick = async () => {
        const sure = confirm("Are you sure you want to delete this user?");
        if (sure) {
            await deleteUser(props.userId);
        }
    }

    return (
        <button className="text-red-500 hover:text-red-700 cursor-pointer py-1 px-4" title="Delete" onClick={handleClick}>
            <FaTrash />
        </button>
    ); 
}
