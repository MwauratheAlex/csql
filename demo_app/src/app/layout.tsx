import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import "../globals.css";
import { FaDatabase } from "react-icons/fa6";

const geistSans = Geist({
    variable: "--font-geist-sans",
    subsets: ["latin"],
});

const geistMono = Geist_Mono({
    variable: "--font-geist-mono",
    subsets: ["latin"],
});

export const metadata: Metadata = {
    title: "CSQL Demo App",
    description: "CSQL Demo App for a database engine implemented in C",
};

export default function RootLayout({
    children,
}: Readonly<{
    children: React.ReactNode;
}>) {
    return (
        <html lang="en">
            <body
                className={`${geistSans.variable} ${geistMono.variable} antialiased h-screen 
                    flex flex-col bg-white`}
            >
                <nav className="bg-slate-900 text-white p-4 shadow-lg">
                    <div className="container mx-auto flex justify-between items-center">
                        <div className="flex items-center gap-3">
                            <FaDatabase size={20} className="fill-emerald-400" />
                            <h1 className="text-xl font-bold tracking-tight">CSQL <span
                                className="text-slate-400 font-normal text-sm">Demo</span></h1>
                        </div>
                    </div>
                </nav>

                {children}
            </body>
        </html>
    );
}


























