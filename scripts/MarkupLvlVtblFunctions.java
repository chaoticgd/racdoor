//TODO write a description for this script
//@author 
//@category Ratchet & Clank
//@keybinding 
//@menupath 
//@toolbar 

import java.util.ArrayList;
import java.util.List;

import ghidra.app.cmd.function.CreateFunctionCmd;
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

public class MarkupLvlVtblFunctions extends GhidraScript {

	public void run() throws Exception {
		boolean interactive = askYesNo("Interactive Mode", "Interactive mode?");
		Address mobyBegin = null;
		Address mobyEnd = null;
		Address camBegin = null;
		Address camEnd = null;
		Address soundBegin = null;
		Address soundEnd = null;
		
		if (interactive) {
			mobyBegin = askAddress("lvl.vtbl", "Address:");
			mobyEnd = askAddress("lvl.vtbl end", "Address:");
			camBegin = askAddress("lvl.camvtbl", "Address:");
			camEnd = askAddress("lvl.camvtbl end", "Address:");
			soundBegin = askAddress("lvl.sndvtbl", "Address:");
			soundEnd = askAddress("lvl.sndvtbl end", "Address:");
		} else {
			for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
				String name = block.getName();
				if (name.endsWith(".vtbl")) {
					mobyBegin = block.getStart();
					mobyEnd = block.getEnd();
				} else if (name.endsWith(".camvtbl")) {
					camBegin = block.getStart();
					camEnd = block.getEnd();
				} else if (name.endsWith(".sndvtbl")) {
					soundBegin = block.getStart();
					soundEnd = block.getEnd();
				}
			}
		}
		
		if(mobyBegin == null || mobyEnd == null) {
			throw new Exception("Cannot find lvl.vtbl block.");
		}
		
		if(camBegin == null || camEnd == null) {
			throw new Exception("Cannot find lvl.camvtbl block.");
		}
		
		if(soundBegin == null || soundEnd == null) {
			throw new Exception("Cannot find lvl.sndvtbl block.");
		}
		
		createMobyFunctions(mobyBegin, mobyEnd);
		createCameraFunctions(camBegin, camEnd);
		createSoundFunctions(soundBegin, soundEnd);
	}

	public void createMobyFunctions(Address start, Address end) throws Exception {
		DataType mobyInstance = new StructureDataType("MobyInstance", 0x100);
		mobyInstance = currentProgram.getDataTypeManager().addDataType(mobyInstance, DataTypeConflictHandler.DEFAULT_HANDLER);
		
		int size = (int) (end.getOffset() - start.getOffset());
		for(int i = 0; i < size; i += 12) {
			int classId = getInt(start.add(i));
			int functionPointer = getInt(start.add(i + 4));
			
			if(classId == -1) {
				break;
			}
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"UpdateMoby_",
					"pMoby",
					mobyInstance,
					classId,
					functionPointer,
					false);
		}
	}
	
	public void createCameraFunctions(Address start, Address end) throws Exception {
		DataType cameraInstance = new StructureDataType("UpdateCam", 0xb0);
		cameraInstance = currentProgram.getDataTypeManager().addDataType(cameraInstance, DataTypeConflictHandler.DEFAULT_HANDLER);
		
		int size = (int) (end.getOffset() - start.getOffset());
		for(int i = 0; i < size; i += 20) {
			int classId = getInt(start.add(i));
			int functionPointer1 = getInt(start.add(i + 4));
			int functionPointer2 = getInt(start.add(i + 8));
			int functionPointer3 = getInt(start.add(i + 12));
			int functionPointer4 = getInt(start.add(i + 16));
			
			if(classId == -1) {
				break;
			}
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"InitCamera_",
					"pCam",
					cameraInstance,
					classId,
					functionPointer1,
					false);
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"ActivateCamera_",
					"pCam",
					cameraInstance,
					classId,
					functionPointer2,
					true);
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"UpdateCamera_",
					"pCam",
					cameraInstance,
					classId,
					functionPointer3,
					false);
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"ExitCamera_",
					"pCam",
					cameraInstance,
					classId,
					functionPointer4,
					false);
		}
	}

	public void createSoundFunctions(Address start, Address end) throws Exception {
		DataType soundInstance = new StructureDataType("SoundInstance", 0x90);
		soundInstance = currentProgram.getDataTypeManager().addDataType(soundInstance, DataTypeConflictHandler.DEFAULT_HANDLER);
		
		int size = (int) (end.getOffset() - start.getOffset());
		for(int i = 0; i < size; i += 8) {
			int classId = getInt(start.add(i));
			int functionPointer = getInt(start.add(i + 4));
			
			if(classId == -1) {
				break;
			}
			
			createOrRenameUpdateFunction(
					start.getAddressSpace(),
					"UpdateSound_",
					"pSound",
					soundInstance,
					classId,
					functionPointer,
					false);
		}
	}
	
	public void createOrRenameUpdateFunction(
			AddressSpace space,
			String namePrefix,
			String parameterName,
			DataType instanceType,
			int classId,
			int functionPointer,
			boolean isActivateCamFunction) throws Exception {
		if(functionPointer == 0) {
			return;
		}
		
		Address address = space.getAddress(functionPointer);
		Function function = this.getFunctionAt(address);
		
		String newFuncName = namePrefix + Integer.toString(classId);
		
		if(function == null) {
			function = createFunction(address, "FUN");
			if(function == null) {
				println("Cannot create " + newFuncName + " at " + Integer.toString(functionPointer));
				return;
			}
		}
		
		if(function.getName().startsWith("FUN")) {
			function.setName(newFuncName, SourceType.IMPORTED);
			
			// Setup the parameter.
			ArrayList<Variable> parameters = new ArrayList<>();
			parameters.add(new ParameterImpl(parameterName, new PointerDataType(instanceType), currentProgram));
			if(isActivateCamFunction) {
				parameters.add(new ParameterImpl("pCompare", new PointerDataType(instanceType), currentProgram));
			}
			function.replaceParameters(parameters, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.IMPORTED);
		} else {
			function.setName(function.getName() + "_" + Integer.toString(classId), SourceType.IMPORTED);
		}
	}
}
