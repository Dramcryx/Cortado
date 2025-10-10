import lldb

def find_property_in_this_sbvalue(sbvalue, propertyName):
    for i in range(sbvalue.num_children):
        ith = sbvalue.GetChildAtIndex(i)
        if ith.GetName() == propertyName:
            return ith
    return None

def get_continuation_from_promise(promise_value):
    """
    Extract the continuation coroutine_handle<> from a promise object.
    Adjust the member name based on your promise structure.
    
    Args:
        promise_value: SBValue representing the promise object
    
    Returns:
        SBValue of the continuation handle, or None
    """
    if promise_value is None:
        return None

    result = None
    for i in range(promise_value.GetNumChildren()):
        child = promise_value.GetChildAtIndex(i)
        if child.GetName() == "m_continuation":
            return child

        result = get_continuation_from_promise(child)
        if result is not None:
            return result

    return None

def get_coroutine_location(handle_value, target):
    """
    Get source location information for a coroutine.
    """
    resume = find_property_in_this_sbvalue(handle_value, "resume")

    if resume is None:
        resume = find_property_in_this_sbvalue(handle_value, "__resume_fn")

    if resume is None:
        return "unknown location"

    # Get symbol and line info for the resume function
    resume_addr = lldb.SBAddress(resume.GetValueAsUnsigned(), target)
    symbol = resume_addr.GetSymbol()
    line_entry = resume_addr.GetLineEntry()
    
    location_parts = []
    
    if symbol and symbol.IsValid():
        # Demangle the symbol name for readability
        symbol_name = symbol.GetName()
        location_parts.append(f"function: {symbol_name}")
    
    if line_entry and line_entry.IsValid():
        file_spec = line_entry.GetFileSpec()
        line = line_entry.GetLine()
        location_parts.append(f"{file_spec.GetFilename()}:{line}")
    
    return " | ".join(location_parts) if location_parts else f"address: 0x{resume_ptr:x}"


def build_async_stack(handle_value, target, max_depth=50):
    """
    Build the async call chain by traversing continuations.
    
    Args:
        handle_value: Starting coroutine_handle
        target: SBTarget
        max_depth: Maximum depth to prevent infinite loops
    
    Returns:
        List of (handle, promise, location) tuples
    """
    stack = []
    current_handle = handle_value
    visited_addresses = set()

    for depth in range(max_depth):
        if not current_handle or not current_handle.IsValid():
            break

        current_promise = find_property_in_this_sbvalue(current_handle, "promise")
        if current_promise is None:
            current_promise = find_property_in_this_sbvalue(current_handle, "__promise")

        if current_promise is None:
            break

        frame_addr = current_promise.GetValueAsAddress()
        if frame_addr in visited_addresses:
            stack.append((None, None, "CYCLE DETECTED"))
            break
        visited_addresses.add(frame_addr)
        
        # Get location info
        location = get_coroutine_location(current_handle, target)
        
        stack.append((current_handle, current_promise, location))

        # Get continuation from promise
        current_handle = get_continuation_from_promise(current_promise)
    
    return stack

def print_async_stack(stack, result):
    """
    Pretty print the async call stack.
    """
    result.AppendMessage(f"\nAsync Call Stack ({len(stack)} frames):\n")
    result.AppendMessage("=" * 80)
    
    for i, (handle, promise, location) in enumerate(stack):
        result.AppendMessage(f"\nFrame #{i}:")
        result.AppendMessage(f"  Location: {location}")
        
        if promise and promise.IsValid():
            promise_type = promise.GetType().GetName()
            result.AppendMessage(f"  Promise Type: {promise_type}")
            
            # Show promise details if needed
            # Uncomment to see full promise contents:
            # result.AppendMessage(f"  Promise Value: {promise}")
        
        result.AppendMessage("")

def get_variables_from_command(command, result):
    max_depth = 50
    if command is None or len(command) == 0:
        return ("__coro_frame", max_depth)

    parts = command.split()
    if len(parts) == 1:
        try:
            max_depth = int(parts[0])
            return ("__coro_frame", max_depth)
        except ValueError:
            return (parts[0], max_depth)

    if len(parts) > 2:
        result.AppendMessage(f"Too many arguments!")

    try:
        max_depth = int(parts[1])
    except ValueError:
        result.AppendMessage(f"Warning: Invalid max depth '{parts[1]}', using default 50")
        return (None, None)

    return (parts[0], parts[1])
            

def async_backtrace(debugger, command, result, internal_dict):
    """
    LLDB command to print async call stack from a coroutine handle.
    Usage: async_bt <coroutine_handle_variable>
    """
    target = debugger.GetSelectedTarget()
    frame = target.GetProcess().GetSelectedThread().GetSelectedFrame()
    
    # if not command:
    #     result.AppendMessage("Usage: async_bt <coroutine_handle_variable>")
    #     result.AppendMessage("\nExample: async_bt my_continuation")
    #     return
    
    # Parse optional max depth
    (var_name, max_depth) = get_variables_from_command(command, result)

    if var_name is None:
        return

    # Find the variable
    print(f"var_name {var_name}")
    var = frame.FindVariable(var_name)
    if not var or not var.IsValid():
        # Try as an expression
        var = frame.EvaluateExpression(var_name)
        if not var or not var.IsValid():
            result.AppendMessage(f"Variable or expression '{var_name}' not found")
            return

    result.AppendMessage(f"Building async stack from: {var_name}")
    result.AppendMessage(f"Type: {var.GetType().GetName()}")
    
    # Build and print the stack
    stack = build_async_stack(var, target, max_depth)
    
    if not stack:
        result.AppendMessage("No continuation chain found")
        return
    
    print_async_stack(stack, result)

def __lldb_init_module(debugger, internal_dict):
    """
    Initialize the LLDB module and register commands.
    """
    debugger.HandleCommand(
        'command script add -f commands.async_backtrace async_bt'
    )
    print('The "async_bt" command has been installed')
    print('Usage: async_bt <coroutine_handle_variable> [max_depth]')
