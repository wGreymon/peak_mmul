

set RtlHierarchyInfo {[
	{"ID" : "0", "Level" : "0", "Path" : "`AUTOTB_DUT_INST", "Parent" : "", "Child" : ["1", "3", "4", "5", "6", "7", "8"],
		"CDFG" : "mm2s",
		"Protocol" : "ap_ctrl_chain",
		"ControlExist" : "1", "ap_start" : "1", "ap_ready" : "1", "ap_done" : "1", "ap_continue" : "1", "ap_idle" : "1", "real_start" : "0",
		"Pipeline" : "None", "UnalignedPipeline" : "0", "RewindPipeline" : "0", "ProcessNetwork" : "0",
		"II" : "0",
		"VariableLatency" : "1", "ExactLatency" : "-1", "EstimateLatencyMin" : "-1", "EstimateLatencyMax" : "-1",
		"Combinational" : "0",
		"Datapath" : "0",
		"ClockEnable" : "0",
		"HasSubDataflow" : "0",
		"InDataflowNetwork" : "0",
		"HasNonBlockingOperation" : "0",
		"IsBlackBox" : "0",
		"Port" : [
			{"Name" : "gmem", "Type" : "MAXI", "Direction" : "I",
				"BlockSignal" : [
					{"Name" : "gmem_blk_n_AR", "Type" : "RtlSignal"}],
				"SubConnect" : [
					{"ID" : "1", "SubInstance" : "grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Port" : "gmem", "Inst_start_state" : "76", "Inst_end_state" : "77"}]},
			{"Name" : "mem", "Type" : "None", "Direction" : "I"},
			{"Name" : "s_V_data_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s",
				"SubConnect" : [
					{"ID" : "1", "SubInstance" : "grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Port" : "s_V_data_V", "Inst_start_state" : "76", "Inst_end_state" : "77"}]},
			{"Name" : "s_V_keep_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s",
				"SubConnect" : [
					{"ID" : "1", "SubInstance" : "grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Port" : "s_V_keep_V", "Inst_start_state" : "76", "Inst_end_state" : "77"}]},
			{"Name" : "s_V_strb_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s",
				"SubConnect" : [
					{"ID" : "1", "SubInstance" : "grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Port" : "s_V_strb_V", "Inst_start_state" : "76", "Inst_end_state" : "77"}]},
			{"Name" : "s_V_last_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s",
				"SubConnect" : [
					{"ID" : "1", "SubInstance" : "grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Port" : "s_V_last_V", "Inst_start_state" : "76", "Inst_end_state" : "77"}]},
			{"Name" : "size", "Type" : "None", "Direction" : "I"}]},
	{"ID" : "1", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102", "Parent" : "0", "Child" : ["2"],
		"CDFG" : "mm2s_Pipeline_VITIS_LOOP_14_1",
		"Protocol" : "ap_ctrl_hs",
		"ControlExist" : "1", "ap_start" : "1", "ap_ready" : "1", "ap_done" : "1", "ap_continue" : "0", "ap_idle" : "1", "real_start" : "0",
		"Pipeline" : "None", "UnalignedPipeline" : "0", "RewindPipeline" : "0", "ProcessNetwork" : "0",
		"II" : "0",
		"VariableLatency" : "1", "ExactLatency" : "-1", "EstimateLatencyMin" : "-1", "EstimateLatencyMax" : "-1",
		"Combinational" : "0",
		"Datapath" : "0",
		"ClockEnable" : "0",
		"HasSubDataflow" : "0",
		"InDataflowNetwork" : "0",
		"HasNonBlockingOperation" : "0",
		"IsBlackBox" : "0",
		"Port" : [
			{"Name" : "gmem", "Type" : "MAXI", "Direction" : "I",
				"BlockSignal" : [
					{"Name" : "gmem_blk_n_R", "Type" : "RtlSignal"}]},
			{"Name" : "size", "Type" : "None", "Direction" : "I"},
			{"Name" : "sext_ln14", "Type" : "None", "Direction" : "I"},
			{"Name" : "add_ln20", "Type" : "None", "Direction" : "I"},
			{"Name" : "s_V_data_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s",
				"BlockSignal" : [
					{"Name" : "s_TDATA_blk_n", "Type" : "RtlSignal"}]},
			{"Name" : "s_V_keep_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s"},
			{"Name" : "s_V_strb_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s"},
			{"Name" : "s_V_last_V", "Type" : "Axis", "Direction" : "O", "BaseName" : "s"}],
		"Loop" : [
			{"Name" : "VITIS_LOOP_14_1", "PipelineType" : "UPC",
				"LoopDec" : {"FSMBitwidth" : "1", "FirstState" : "ap_ST_fsm_pp0_stage0", "FirstStateIter" : "ap_enable_reg_pp0_iter0", "FirstStateBlock" : "ap_block_pp0_stage0_subdone", "LastState" : "ap_ST_fsm_pp0_stage0", "LastStateIter" : "ap_enable_reg_pp0_iter1", "LastStateBlock" : "ap_block_pp0_stage0_subdone", "QuitState" : "ap_ST_fsm_pp0_stage0", "QuitStateIter" : "ap_enable_reg_pp0_iter0", "QuitStateBlock" : "ap_block_pp0_stage0_subdone", "OneDepthLoop" : "0", "has_ap_ctrl" : "1", "has_continue" : "0"}}]},
	{"ID" : "2", "Level" : "2", "Path" : "`AUTOTB_DUT_INST.grp_mm2s_Pipeline_VITIS_LOOP_14_1_fu_102.flow_control_loop_pipe_sequential_init_U", "Parent" : "1"},
	{"ID" : "3", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.control_s_axi_U", "Parent" : "0"},
	{"ID" : "4", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.gmem_m_axi_U", "Parent" : "0"},
	{"ID" : "5", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.regslice_both_s_V_data_V_U", "Parent" : "0"},
	{"ID" : "6", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.regslice_both_s_V_keep_V_U", "Parent" : "0"},
	{"ID" : "7", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.regslice_both_s_V_strb_V_U", "Parent" : "0"},
	{"ID" : "8", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.regslice_both_s_V_last_V_U", "Parent" : "0"}]}
