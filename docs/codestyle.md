# Coding Style

Alright, I'm outlining the coding style for snow-palm as best I can right now so it won't be wildly confusing later.  This is to ensure some manner of uniformity in snow-palm code, but really is just my personal preferences outlined in a document, since I'm the only person working on it.

## Indentation

* Hard tabs
* Tab width of four characters/spaces in editors

Indentation is done with hard tabs, each tab is four spaces in length.  If possible, trim all whitespace from lines where it's not needed (git doesn't like it), but otherwise it doesn't matter too much if there's whitespace on a blank line.

## Brace Positions

The first opening brace should ideally go on its own line.  For example:

	int main(int argc, char **argv)
	{
		/* ... */
	}

The closing brace _always_ gets its own line.

An exception to this is when the braces follow assignment.  In this case, you should place the opening brace on the same line as the assignment.  For example:

	const class_t _some_class = {
		/* ... */
	};

Opening braces after the first should remain on the same line as the control statement that introduces them.  So, `if (condition) {` and so on is correct.

If introducing a block without a control statement, the opening brace should be on its own line with a comment explaining its purpose _after_ the brace, such as `{  /* event handling */`.  Use these sparingly, since they tend to be pointlessly confusing.



## Naming Convention

### Identifiers in General

All identifiers, generally speaking, follow a naming convention similar to the C standard library: lowercase letters separated by spaces.  If necessary, they may also have numbers, though I recommend only using them when it makes sense or you're specifying something like the number of dimensions to a structure or array.  The following are all valid examples of common identifiers:

* `object_t`
* `object_store_weak`
* `vec3_dot_product`
* `tag`
* ...and so on...


### Functions

Function names follow the rules of identifiers above.  If possible, avoid lengthy function names, but also avoid abbreviations that are not logical (this varies from person to person, so use your best judgment).  For example, `msg` is understood to be `message`, but `lkr` is not an acceptable abbreviation for `locker` (I don't expect `locker` to show up in the code, though).

Numbers should be avoided unless they make sense in the context of the function name.  (i.e., `mat4_set_axes3`)

For some examples of function names:

* `object_store_weak`
* `entity_set_name`
* `mat4_inverse_orthogonal`

#### Method-Like Functions

For functions that act on objects, they should be prefixed with the class name and an underscore.  For example, `object_store_weak`.

If a class name is lengthy, consider using an abbreviation.  Remember to avoid abbreviations that do not make sense.  As a somewhat extreme example, `component_t` would have a method `com_msg_all_of_kind`.


### Structures

All structure names follow the same rules as identifiers, albeit with certain prefixes/suffixes.

#### `union`

A `union` must be prefixed with `u_`.  All `union` declarations must include an accompanying `typedef`.

#### `struct`

A `struct` must be prefixed with `s_`.  All `struct` declarations must include an accompanying `typedef`.

#### `typedef`

A `typedef` always has the suffix `_t` and may either inherit the original typename sans prefix (if it's a `struct` or `union`) or otherwise adopt its own name.  A `typedef` for a `struct` or `union` should be on a line preceding the actual struct declaration.  For example:

	typedef struct s_object object_t;

	struct s_object
	{
		/* members */
	};

#### `enum`

Though not really a structure in the same sense as a `union` or `struct`, the convention in snow-palm is to leave enums themselves unnamed and wrap them in a `typedef`.  For example:

	typedef enum
	{
		BUFFER_DIRTY,
		BUFFER_CLEAN,
		BUFFER_CLOSED,
		BUFFER_OPEN,
		BUFFER_DEFENESTRATED
	} buffer_state_t;

#### Member Variables

Member variables follow the same convention as general identifiers, with one exception: if the variable is considered very private or internal, its name should be prefixed with `prv_`.

#### Class Names

When defining a new class using `class_t` (or a derivative of that type), you should define a global named `g_<classname>_class` followed by a macro `<classname>_class` for the address of the class.  For example:

	extern const class_t g_list_class;
	#define list_class (&g_list_class)

The reason for this is mainly ease of use.  `object_new()` takes a `class_t *`, so it is much more useful to treat it as though it's always pointing to the class.


### Macros

#### Constant Macros

Macros that are used as constants are in all uppercase, with words separated by underscores.  For example:

	#define NDEBUG 0
	#define USE_PTHREADS 1

#### Function Macros

Macros that behave like functions follow the same naming convention as functions.  Macro arguments should be entirely uppercase, however.  For example:

	#define vec3_expand(V) (V)[0], (V)[1], (V)[2]


### Variables

#### Global Variables

Global variables follow general identifier conventions, albeit with a prefix of `g_`.  So, a global list of entities would be `g_entity_list` or `g_entities` (depending on your preference).  The preferred length for global variables is two to three words, though the rule is that you should be as clear as you can about what the variable is for in its name.  It's a global, so if it pops up in other unrelated bits of code, it should be fairly easy to determine what it is and where it came from (though you can always `grep` for it, pretend nobody has `grep`).

#### Local Variables

Local variables don't necessarily require any particular style.  Preferably follow the style for identifiers in general, but if it's on the stack and its scope is very limited, it isn't entirely necessary to worry about naming as long as it's clear what it's for.

Variable names such as `i`, `j`, `k` and so on that are fairly common as iterators should be avoided - if you need a variable name for an index, use `index`.  Better yet, use two words: a word to describe the index and `index`, such as `entity_index`.  Doing this ensures that the purpose of your variable is clear and its usage in the code that follows is easy to track.

#### Constants

Constants follow the same rules as general identifiers.  They must be prefixed with `c_`.  So, for example, `c_err_no_name` would be a valid name for an error constant.

## What Goes

### Semi-Private Functions

If a function needs to be part of the public API, but should not generally be used except in edge cases, it's probably bad design, for one.  However, if it's considered unavoidable, leave a comment by the function prototype in the header stating that the function probably shouldn't be used.

### Member Access

In general, members should be considered read-only in general code.  There are some small exceptions to this, however:

* `listnode_t.obj` is write-able if the list stores weak references.  If it stores strong references, you should modify the list through the appropriate methods.
* `mapnode_t.p` (an opaque value associated with the map) can be considered write-able when the `mapops_t` associated with the map is the default.
	
	Again, it is better to modify it through the appropriate method (i.e., `map_insert`) rather than something else.
* `array_t.buffer` is the same buffer accessed via `array_buffer(array_t *)`, so it is not unreasonable to access it directly.  If you need to know the exact size of the buffer, however, it is better to access it via `array_buffer(array_t *)`.

