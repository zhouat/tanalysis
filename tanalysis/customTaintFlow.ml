open Cil_types
open TaintTyping
open TaintGamma

module TaintComputer(Param:sig
                        (* The int key hashtable that holds the environment *)
                        (* for each statement in the function. *)
	                    val stmt_envs : statementsEnvironment                        
	                 end) = struct


    (* Tests if the old environment and the new environment are the same. *)
    let test_for_change old_ new_ =
        match Gamma.compare old_ new_ with
            | true -> (true, old_)
            | false -> (false, new_)

    (* Applies the transformations done by a statement to a given environment. *)
    (* Returns the new environment. *)
    let do_stmt stmt new_env =
        new_env

    (* This is the main entry point of the analysis. *)
    (* Params: *)
    (* worklist - the list of statements that will be computed. Initially this *)
    (* must hold only the starting statement *) 
    let rec compute worklist =
        let current_stmt = List.hd worklist in
        (* Foreach predecessor, combine the results. If there aren't any preds *)
        (* then the statements' environment is returned. *)
        let new_env = 
            match List.length current_stmt.preds with
                | 0 ->
                    Inthash.find Param.stmt_envs current_stmt.sid
                | _ 
                    ->
                    let first_pred = List.hd current_stmt.preds in
                    let first_pred_id = first_pred.sid in 
		            List.fold_left
		                (fun env pred_stmt ->
		                    let pred_env = Inthash.find Param.stmt_envs pred_stmt.sid in
		                    Typing.combine env pred_env)
                        (Inthash.find Param.stmt_envs first_pred_id)    
		                current_stmt.preds
        in
        let old_env = Hashtbl.copy (Inthash.find Param.stmt_envs current_stmt.sid) in
        let (changed, env) = test_for_change old_env (do_stmt current_stmt new_env) in 
        match (changed, env) with
            | (false, _) 
                -> 
                    (* Fix point reached for current statement. The successors *)
                    (* aren't added to the worklist. *)
                    Inthash.replace Param.stmt_envs current_stmt.sid env;
                    compute (List.tl worklist)
            | (true, _)
                -> 
                    (* We still haven't reached a fixed point for the statement. *)
                    (* Add the successors to the worklist. *)
                    Inthash.replace Param.stmt_envs current_stmt.sid env;
                    compute (List.concat [List.tl worklist;current_stmt.succs])
                                                                        
end
