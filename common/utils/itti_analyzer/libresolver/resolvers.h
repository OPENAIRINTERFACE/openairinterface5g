#ifndef RESOLVERS_H_
#define RESOLVERS_H_

int resolve_typedefs(types_t **head);

int resolve_struct(types_t **head);

int resolve_pointer_type(types_t **head);

int resolve_field(types_t **head);

int resolve_array(types_t **head);

int resolve_reference(types_t **head);

int resolve_union(types_t **head);

int resolve_file(types_t **head);

int resolve_function(types_t **head);

int search_file(types_t *head, types_t **found, int id);

#endif /* RESOLVERS_H_ */
