'use client'
import {
    FaTableColumns,
    FaTerminal,
} from "react-icons/fa6";
import Link from 'next/link';
import { useEffect,
    useRef,
    useState } from "react";
import { executeRawSQL } from "@/actions";

export default function TerminalPage() {
    return (
        <main className="mx-auto grow overflow-y-scroll border w-full">
            <div className="container  mx-auto py-6">
                <Header />
            </div>
            <Terminal />
        </main>
    );
}

export const Terminal = () => {
    const [lines, setLines] = useState<string[]>([
        "CSQL v1.0.0 (x86_64-linux-gnu)",
        "Type 'help' for commands. Enter SQL to execute.",
        "Press 'Enter' key to execute the command after typing it below.",
        ""
    ]);

    const [input, setInput] = useState("");
    const [isPending, setIsPending] = useState(false);
    const [history, setHistory] = useState<string[]>([]);
    const [historyIdx, setHistoryIdx] = useState(-1);

    const bottomRef = useRef<HTMLDivElement>(null);
    const inputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [lines]);

    useEffect(() => {
        if (!isPending) {
            inputRef.current?.focus();
        }
    }, [isPending]);

    const handleContainerClick = () => {
        inputRef.current?.focus();
    };

    const handleKeyDown = async (e: React.KeyboardEvent<HTMLInputElement>) => {
        if (e.key === 'ArrowUp') {
            e.preventDefault();
            if (history.length > 0) {
                const newIdx = Math.min(historyIdx + 1, history.length - 1);
                setHistoryIdx(newIdx);
                setInput(history[history.length - 1 - newIdx]);
            }
            return;
        }

        if (e.key === 'ArrowDown') {
            e.preventDefault();
            if (historyIdx > 0) {
                const newIdx = historyIdx - 1;
                setHistoryIdx(newIdx);
                setInput(history[history.length - 1 - newIdx]);
            } else {
                setHistoryIdx(-1);
                setInput("");
            }
            return;
        }

        if (e.key === 'Enter' && !isPending) {
            const cmd = input.trim();
            if (!cmd) return;

            setInput("");
            setHistoryIdx(-1);
            setHistory(prev => [...prev, cmd]);
            setIsPending(true);

            setLines(prev => [...prev, `csql > ${cmd}`]);

            const lowerCmd = cmd.toLowerCase();
            if (lowerCmd === 'clear') {
                setLines([
                    "CSQL v1.0.0 (x86_64-linux-gnu)",
                    "Type 'help' for commands. Enter SQL to execute.",
                    "Press 'Enter' key to run the command after typing it.",
                    ""
                ]);
                setIsPending(false);
                return;
            }
            if (lowerCmd === 'help') {
                const helpMessage = [
                    "--- Data Definition ---",
                    "  CREATE TABLE <table> (<col> <type>, ...);",
                    "    Types: int, text",
                    "  CREATE INDEX <name> ON <table> (<col>);",
                    "",
                    "--- Data Manipulation ---",
                    "  INSERT INTO <table> VALUES (val1, val2, ...);",
                    "  UPDATE <table> SET col='val' WHERE col=val;",
                    "  DELETE FROM <table> WHERE col=val;",
                    "",
                    "--- Querying ---",
                    "  SELECT * FROM <table>;",
                    "  SELECT id, name FROM <table>;",
                    "  SELECT * FROM <table> WHERE col=val;",
                    "  SELECT * FROM <t1> JOIN <t2> ON t1.c1=t2.c2;",
                    "",
                    "--- Other Commands ---",
                    "  clear   : Clears the screen",
                    "  help    : Shows this message",
                    ""
                ]; setLines(prev => [...prev, ...helpMessage]);
                setIsPending(false);
                return;
            }

            try {
                const result = await executeRawSQL(cmd);
                setLines(prev => [...prev, result]);
            } catch (err) {
                setLines(prev => [...prev, "Error: Connection failed."]);
            } finally {
                setIsPending(false);
            }
        }
    };

    return (
        <div id="view-terminal" 
            onClick={handleContainerClick}
            className="h-150 flex flex-col bg-slate-900 rounded-xl shadow-2xl overflow-hidden
            border border-slate-700 font-mono text-sm shadow-black container mx-auto cursor-text">
            <div className="bg-slate-800 px-4 py-2 flex items-center gap-2 border-b border-slate-700 select-none">
                <div className="flex gap-1.5">
                    <div className="w-3 h-3 rounded-full bg-red-500"></div>
                    <div className="w-3 h-3 rounded-full bg-yellow-500"></div>
                    <div className="w-3 h-3 rounded-full bg-green-500"></div>
                </div>
                <div className="text-slate-400 ml-3 text-xs">csql-cli — remote connection</div>
            </div>

            <div id="terminal-output" className="grow p-4 overflow-y-auto text-slate-300 
            scrollbar-thin scrollbar-thumb-slate-700 scrollbar-track-transparent">
                {lines.map((line, i) => (
                    <div key={i} className="whitespace-pre-wrap mb-1 wrap-break-word">
                        {line}
                    </div>
                ))}
                <div ref={bottomRef} />
            </div>

            <div className="bg-slate-800 p-4 flex items-center gap-2 border-t border-slate-700">
                <span className="text-emerald-500 font-bold select-none">csql &gt;</span>
                <input 
                    ref={inputRef}
                    type="text" 
                    id="terminal-input" 
                    autoComplete="off"
                    value={input}
                    onChange={(e) => setInput(e.target.value)}
                    onKeyDown={handleKeyDown}
                    readOnly={isPending}
                    className={`bg-transparent border-none outline-none text-white grow 
                        font-mono placeholder-slate-600 ${isPending ? 'opacity-50' : 'opacity-100'}`}
                    placeholder="SELECT * FROM users;" 
                    autoFocus
                />
            </div>
        </div>
    );
};

const Header = () => {
    return (
        <header className="flex gap-4 border-b border-slate-200">
            <Link id="tab-visual" href="/"
                className="pb-3 px-2 border-b-2 border-transparent hover:text-slate-700 
                text-slate-500 transition-all flex items-center"> 
                <FaTableColumns size={18} />
            </Link>
            <Link href="/terminal" id="tab-terminal"
                className="pb-3 px-2 border-b-2 border-slate-900 font-semibold 
                text-slate-900 transition-all flex items-center gap-2">
                <FaTerminal size={18} /> SQL Terminal
            </Link>
        </header>
    );
}
