// Given the address of a function in one level overlay, find it in all the others.
//@author chaoticgd
//@category Ratchet & Clank
//@keybinding 
//@menupath 
//@toolbar 

import java.util.ArrayList;

import ghidra.app.script.GhidraScript;
import ghidra.feature.fid.hash.FidHashQuad;
import ghidra.feature.fid.service.FidService;
import ghidra.program.model.mem.*;
import ghidra.program.model.lang.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.data.ISF.*;
import ghidra.program.model.util.*;
import ghidra.util.NumericUtilities;
import ghidra.program.model.reloc.*;
import ghidra.program.model.data.*;
import ghidra.program.model.block.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.scalar.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;

public class FindFunctionForAllLevels extends GhidraScript {
	public class Column {
		boolean isOverlay = false;
		
		// We only need to know about one of the level numbers for the column
		// since the others will have identical overlays.
		int level = -1;
		
		String heading = "";
		String value = "";
		
		ArrayList<String> multi = new ArrayList<String>();
	}
	
	FidService fid;
	
	public void run() throws Exception {
		fid = new FidService();
		
		String csvHeader = this.askString("Enter CSV Header", "Line:");
		if(csvHeader.charAt(csvHeader.length() - 1) == ' ') {
			csvHeader = csvHeader.substring(0, csvHeader.length() - 1);
		}
		
		ArrayList<Column> columns = parseHeader(csvHeader);

		String csvSymbol = this.askString("Enter CSV Symbol", "Line:");
		if(csvSymbol.charAt(csvSymbol.length() - 1) == ' ') {
			csvSymbol = csvSymbol.substring(0, csvSymbol.length() - 1);
		}
		
		parseSymbol(columns, csvSymbol);
		
		//for (Column col : columns) {
		//	println(col.heading + ": " + Integer.toString(col.level) + " " + col.value);
		//}
		
		FidHashQuad targetHash = getTargetFidHash(columns);
		if (targetHash == null) {
			throw new Exception("Cannot compute target hash.");
		}
		
		println("Target Hash: " + targetHash.toString());
		
		findFunctions(columns, targetHash);
		printSymbol(columns);
	}
	
	FidHashQuad getTargetFidHash(ArrayList<Column> columns) throws Exception {
		for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
			if (!block.getName().startsWith("lvl") || block.getName().charAt(3) == '.') {
				continue;
			}
			
			int level = parseInt(block.getName(), 3);
			
			Column column = null;
			for (Column col : columns) {
				if (col.level == level && !col.value.isEmpty()) {
					column = col;
				}
			}
			if (column == null) {
				continue;
			}
			
			long addr = Long.parseLong(column.value, 16);
			Address funcAddr = block.getStart().getAddressSpace().getAddress(addr);
			Function function = getFunctionAt(funcAddr);
			FidHashQuad hash = fid.hashFunction(function);
			if (hash == null) {
				throw new Exception("Failed to compute hash for addr " + Long.toString(addr) + " in " + block.getName());
			}
			return hash;
		}
		return null;
	}
	
	void findFunctions(ArrayList<Column> columns, FidHashQuad targetHash) throws Exception {
		for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
			if (!block.getName().startsWith("lvl") || block.getName().charAt(3) == '.') {
				continue;
			}
			
			int level = parseInt(block.getName(), 3);
			
			Column column = null;
			for (Column col : columns) {
				if (col.isOverlay && col.level == level) {
					column = col;
					break;
				}
			}
			
			if (column == null) {
				continue;
			}
			
			AddressSet range = new AddressSet(block.getAddressRange());
			FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(range, true);
			for (Function function : functions) {
				if (monitor.isCancelled()) {
					throw new Exception("Cancelled.");
				}
				
				FidHashQuad hash = fid.hashFunction(function);
				if (hash != null && hash.getFullHash() == targetHash.getFullHash()) {
					String value = Long.toString(function.getEntryPoint().getOffset(), 16);
					while (value.length() < 8) {
						value = "0" + value;
					}
					if (column.value.isEmpty()) {
						column.value = value;
					}
					column.multi.add(value);
					println("Found func " + function.getName() + " in " + block.getName());
				}
			}
		}
	}
	
	void printSymbol(ArrayList<Column> columns) throws Exception {
		println("\n\nSINGLE:\n\n");
		for (int i = 0; i < columns.size(); i++) {
			Column column = columns.get(i);
			print(column.value);
			if (i != columns.size() - 1) {
				print(",");
			}
		}
		print("\n\nMULTI:\n\n");
		for (int i = 0; i < 100; i++) {
			boolean occupied = false;
			for (int j = 0; j < columns.size(); j++) {
				Column column = columns.get(j);
				if(column.isOverlay) {
					if (i < column.multi.size()) {
						print(column.multi.get(i));
						occupied = true;
					}
				} else {
					print(column.value);
				}
				if (j != columns.size() - 1) {
					print(",");
				}
			}
			print("\n");
			if (!occupied) {
				break;
			}
		}
		print("\n");
	}
	
	ArrayList<Column> parseHeader(String line) throws Exception {
		ArrayList<Column> columns = new ArrayList<Column>();
		
		int pos = 0;
		while (pos < line.length()) {
			boolean quoted = false;
			if (line.charAt(pos) == '"') {
				quoted = true;
				pos++;
			}
			
			if (pos == line.length()) {
				break;
			}

			Column column = new Column();
			
			int startPos = pos;
			
			int c = line.charAt(pos);
			switch (c) {
			case '[':
			case '{':
				pos++;
				column.isOverlay = true;
				column.level = parseInt(line, pos);
				while (c != ']' && c != '}') {
					pos++;
					if (pos == line.length()) {
						break;
					}
					c = line.charAt(pos);
				}
				if (pos < line.length()) {
					pos++;
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				column.isOverlay = true;
				column.level = parseInt(line, pos);
			default:
				while (c != ',') {
					pos++;
					if(pos == line.length()) {
						break;
					}
					c = line.charAt(pos);
				}
			}
			
			column.heading = line.substring(startPos, pos);
			
			columns.add(column);
			
			if (pos == line.length()) {
				break;
			}
			
			if (quoted) {
				if (line.charAt(pos) != '"') {
					throw new Exception("Failed to parse CSV header (no terminating quote).");
				}
				pos++;
			}
			
			if (pos == line.length()) {
				break;
			}
			
			if (line.charAt(pos) == ',') {
				pos++;
			}
		}
		
		return columns;
	}
	
	void parseSymbol(ArrayList<Column> columns, String line) throws Exception {
		int columnIndex = 0;
		
		int pos = 0;
		while (pos < line.length()) {
			boolean quoted = false;
			if (line.charAt(pos) == '"') {
				quoted = true;
				pos++;
				continue;
			}
			
			if (pos == line.length()) {
				break;
			}
			
			if (columnIndex >= columns.size()) {
				break;
			}
			
			Column column = columns.get(columnIndex++);
			
			int startPos = pos;
			int c = line.charAt(pos);
			while (quoted ? (c != '"') : (c != ',')) {
				pos++;
				if(pos == line.length()) {
					break;
				}
				c = line.charAt(pos);
			}
			column.value = line.substring(startPos, pos);
			
			if (pos == line.length()) {
				break;
			}
			
			if (quoted) {
				if (line.charAt(pos) != '"') {
					throw new Exception("Failed to parse CSV symbol (no terminating quote).");
				}
				pos++;
			}
			
			if (pos == line.length()) {
				break;
			}
			
			if (line.charAt(pos) == ',') {
				pos++;
			}
		}
	}
	
	int parseInt(String input, int pos) {
		int endPos = pos;
		while (endPos < input.length()) {
			int c = input.charAt(endPos);
			if (c < '0' || c > '9') {
				break;
			}
			endPos++;
		}
		String string = input.substring(pos, endPos);
		int parsed = Integer.parseInt(string);
		return parsed;
	}
}
