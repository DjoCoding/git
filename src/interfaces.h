#ifndef INTERFACES_H_
#define INTERFACES_H_

#define IMPLEMENTS

// interface Hashable {
// 		void self_hash(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);
// }

#define Hashable

// interface Writable {
// 		Result self_write_to_file(Self *self, char *file_path, StringBuilder *sb);
// }

#define Writable

// interface Loadable {
// 		Result self_load_from_file(char *file_path, StringBuilder *sb);
// }

#define Loadable

#endif