// Find a function in the current program given its full FID hash.
//@author chaoticgd
//@category Ratchet & Clank
//@keybinding 
//@menupath 
//@toolbar 

import ghidra.app.script.GhidraScript;
import ghidra.feature.fid.hash.FidHashQuad;
import ghidra.feature.fid.service.FidService;
import ghidra.program.model.lang.protorules.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.lang.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.data.ISF.*;
import ghidra.program.model.util.*;
import ghidra.program.model.reloc.*;
import ghidra.program.model.data.*;
import ghidra.program.model.block.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.scalar.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.util.NumericUtilities;

public class FindFunctionInCurrentProgramByHash extends GhidraScript {

	public void run() throws Exception {
		FidService fid = new FidService();
		String fullHashString = askString("Enter FID Hash", "Full hash as hex:");
		long fullHash = NumericUtilities.parseHexLong(fullHashString);
		FunctionManager functionManager = currentProgram.getFunctionManager();
		for (Function function : functionManager.getFunctions(true)) {
			FidHashQuad hashQuad = fid.hashFunction(function);
			if (hashQuad != null && hashQuad.getFullHash() == fullHash) {
				println("Found function " + function.getName());
				break;
			}
		}
	}

}
