// Find relative branch instructions that can be modified to jump into memory card data.
//@author chaoticgd
//@category Ratchet & Clank
//@keybinding 
//@menupath 
//@toolbar 

import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
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

public class FindMemcardBranches extends GhidraScript {
	public class MemcardBlock {
		public int address;
		public int size;
		public int iff;
	};
	
	public void run() throws Exception {
		ArrayList<MemcardBlock> blocks = new ArrayList<>();
		
		int levelCount = askInt("Level Count", "Count:");
		if (levelCount < 0 || levelCount > 64)
			throw new Exception("Bad level count.");
		
		boolean writeTwoBytes = askYesNo("Two bytes instead of one byte?", "?");
		String valueStr = askString("Value to write", "In hex:");
		long valueLong = Long.parseLong(valueStr, 16);
		short valueToWrite = (short) valueLong;
		
		List<Symbol> gamedata = currentProgram.getSymbolTable().getLabelOrFunctionSymbols("mc_gamedata", null);
		if (gamedata == null || gamedata.size() != 1)
			throw new Exception("Cannot find mc_gamedata symbol.");
		
		List<Symbol> leveldata = currentProgram.getSymbolTable().getLabelOrFunctionSymbols("mc_leveldata", null);
		if (leveldata == null || leveldata.size() != 1)
			throw new Exception("Cannot find mc_leveldata symbol.");

		processMcDataList(blocks, gamedata.get(0).getAddress(), 1);
		processMcDataList(blocks, leveldata.get(0).getAddress(), levelCount);
		
		scanFunctions(blocks, writeTwoBytes, valueToWrite);
	}
	
	public void processMcDataList(ArrayList<MemcardBlock> blocks, Address addr, int sizeMultiplier) throws Exception {
		for (int i = 0; i < 64; i++) {
			int address = getInt(addr.add(0));
			int size = getInt(addr.add(4));
			int iff = getInt(addr.add(8));
			int mark = getInt(addr.add(12));
			
			if (address == 0)
				break;
			
			MemcardBlock block = new MemcardBlock();
			block.address = address;
			block.size = size * sizeMultiplier;
			block.iff = iff;
			blocks.add(block);
			
			addr = addr.add(16);
		}
	}
	
	public class Opcode {
		public int opcode;
		public int regimm;
		
		Opcode(int o) {
			opcode = o;
		}
		
		Opcode(int o, int r) {
			opcode = o;
			regimm = r;
		}
	};
	
	public void scanFunctions(ArrayList<MemcardBlock> blocks, boolean writeTwoBytes, short valueToWrite) throws Exception {
		Opcode opcodes[] = {
			new Opcode(0b000100), // beq
			new Opcode(0b010100), // beql
			new Opcode(0b000001, 0b00001), // bgez
			new Opcode(0b000001, 0b10001), // bgezal
			new Opcode(0b000001, 0b10011), // bgezall
			new Opcode(0b000001, 0b00011), // bgezl
			new Opcode(0b000111), // bgtz
			new Opcode(0b010111), // bgtzl
			new Opcode(0b000110), // blez
			new Opcode(0b010110), // blezl
			new Opcode(0b000001, 0b00000), // bltz
			new Opcode(0b000001, 0b10000), // bltzal
			new Opcode(0b000001, 0b10010), // bltzall
			new Opcode(0b000001, 0b00010), // bltzl
			new Opcode(0b000101), // bne
			new Opcode(0b010101) // bnel
		};
		
		int opmask = 0b111111 << 26;
		int regimmmask = 0b11111 << 16;
		
		StringBuilder output = new StringBuilder();
		
		for (Function function : currentProgram.getFunctionManager().getFunctions(true) ) {
			for (Address ptr = function.getEntryPoint(); function.getBody().contains(ptr); ptr = ptr.add(4)) {
				int insn = getInt(ptr);
				
				boolean matches = false;
				for(Opcode opcode : opcodes) {
					if ((insn & opmask) == (opcode.opcode << 26)) {
						if (opcode.opcode != 1 || (insn & regimmmask) == opcode.regimm << 16) {
							matches = true;
							break;
						}
					}
				}
				
				if (matches) {
					short offset = (short) (insn & 0xffff);
					if(writeTwoBytes) {
						offset = valueToWrite;
					} else {
						offset = (short) ((valueToWrite << 8) | (offset & 0xff));
					}
					long target = ptr.getOffset() + (offset << 2) + 4;
					
					for (MemcardBlock block : blocks) {
						if(target >= block.address && target < block.address + block.size) {
							long blockOffset = target - block.address;
							output.append(function.toString() + " " +
									ptr.toString() + " -> " +
									Long.toHexString(target) + " (" +
									block.iff + " + " +
									Long.toHexString(blockOffset) + ")\n");
						}
					}
				}
			}
		}
		
		FileWriter file = new FileWriter("/tmp/found_memcard_branches.txt");
		file.write(output.toString(), 0, output.length());
		file.close();
	}
}
