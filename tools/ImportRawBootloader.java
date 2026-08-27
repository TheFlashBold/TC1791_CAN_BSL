import java.io.File;

import ghidra.app.plugin.core.analysis.AutoAnalysisManager;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.importer.AutoImporter;
import ghidra.app.util.importer.MessageLog;
import ghidra.app.util.opinion.Loaded;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Program;

public class ImportRawBootloader extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 2) {
            printerr("usage: ImportRawBootloader <host-file> <project-name>");
            return;
        }

        File binary = new File(arguments[0]);
        if (!binary.isFile()) {
            printerr("file not found: " + binary);
            return;
        }

        Loaded<Program> loaded = AutoImporter.importAsBinary(
            binary,
            state.getProject(),
            "/",
            currentProgram.getLanguage(),
            currentProgram.getCompilerSpec(),
            this,
            new MessageLog(),
            monitor);
        Program program = loaded.getDomainObject(this);
        int transaction = program.startTransaction("Configure TC1791 PSPR image");
        boolean commit = false;
        try {
            Address base = program.getAddressFactory()
                .getDefaultAddressSpace()
                .getAddress(0xC0000000L);
            program.setImageBase(base, true);
            program.setName(arguments[1]);
            commit = true;
        }
        finally {
            program.endTransaction(transaction, commit);
        }

        AutoAnalysisManager analysis = AutoAnalysisManager.getAnalysisManager(program);
        analysis.initializeOptions();
        analysis.startAnalysis(monitor);
        loaded.save(monitor);

        println("Imported " + program.getName()
            + " base=" + program.getImageBase()
            + " length=" + binary.length()
            + " functions=" + program.getFunctionManager().getFunctionCount());
        loaded.release(this);
    }
}
