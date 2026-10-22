// scripts/ExportToC.java
//
// Ghidra headless post-script for the Unlaunch ARM9 image (Ghidra 12.x).
//
// The program is imported from a generated ELF (see build_arm9_elf / the
// runner) so that the ELF loader supplies the correct load address and entry
// point. We therefore do NOT call clearCodeUnits() -- that would throw away the
// auto-analysis that analyzeHeadless already performed.
//
// Usage (from analyzeHeadless):
//     -postScript ExportToC.java [outputDir] [entryAddress]
//
// Every discovered function is decompiled and written as <name>_<addr>.c into
// outputDir (default /work/asm/decompiled/arm9). An index.tsv summarising
// name, address, size and signature is written alongside it.

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.SourceType;
import java.io.File;
import java.io.PrintWriter;

public class ExportToC extends GhidraScript {

    private static final String DEFAULT_OUT = "/work/asm/decompiled/arm9";

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outDirPath = (args.length >= 1) ? args[0] : DEFAULT_OUT;

        // Make sure the configured entry point is decoded and a function exists
        // there even if the loader's entry symbol was not honoured.
        if (args.length >= 2) {
            try {
                Address entry = toAddr(args[1]);
                if (entry != null && currentProgram.getMemory().contains(entry)) {
                    disassemble(entry);
                    if (getFunctionAt(entry) == null) {
                        createFunction(entry, "arm9_entry");
                    }
                    println("[*] Seeded entry point at " + entry);
                }
            } catch (Exception e) {
                println("[!] Could not seed entry point: " + e.getMessage());
            }
        }

        File outDir = new File(outDirPath);
        if (!outDir.exists() && !outDir.mkdirs()) {
            println("[!] Could not create output directory " + outDirPath);
            return;
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        decompiler.setSimplificationStyle("decompile");
        if (!decompiler.openProgram(currentProgram)) {
            println("[!] Decompiler could not open program");
            return;
        }

        PrintWriter index = new PrintWriter(new File(outDir, "index.tsv"));
        index.println("name\taddress\tsize\tsignature");

        FunctionIterator funcIter =
            currentProgram.getFunctionManager().getFunctions(true);
        int count = 0;

        while (funcIter.hasNext() && !monitor.isCancelled()) {
            Function func = funcIter.next();
            if (func.isExternal() || func.isThunk()) {
                continue;
            }

            DecompileResults res = decompiler.decompileFunction(func, 60, monitor);
            if (res == null || !res.decompileCompleted()) {
                continue;
            }

            String cCode = res.getDecompiledFunction().getC();
            String funcName = func.getName();
            Address addr = func.getEntryPoint();

            File outFile = new File(outDir, funcName + "_" + addr + ".c");
            PrintWriter writer = new PrintWriter(outFile);
            writer.println("// Function: " + funcName);
            writer.println("// Address:  " + addr);
            writer.println("// Signature: " + func.getPrototypeString(false, false));
            writer.println();
            writer.print(cCode);
            writer.close();

            index.println(
                funcName + "\t" + addr + "\t" + func.getBody().getNumAddresses()
                    + "\t" + func.getPrototypeString(false, false));
            count++;
        }

        index.close();
        decompiler.dispose();
        println("[*] Successfully exported " + count + " ARM9 functions to " + outDirPath);
    }
}
