

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 1 -r 1755708657 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_1/params_5/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 1 -r 1755708657 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_1/params_5/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 1 -r 1755708657 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_1/params_5/validate/$q.validate.sql; done

# 1755708640
# 1755708649
1755708657