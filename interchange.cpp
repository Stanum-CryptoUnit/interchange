
#include "interchange.hpp"


using namespace eosio;


[[eosio::action]] void interchange::cancel(eosio::name username, uint64_t id){
  require_auth(username);

  changes_index changes(_me, _me.value);
  auto change = changes.find(id);

  eosio::check(change != changes.end(), "Change object is not found");
  eosio::check(change -> status == "process"_n, "Change object is proccessed or canceled");

  action(
      permission_level{ _me, "active"_n },
      _token_contract, "transfer"_n,
      std::make_tuple( _me, change -> username, change -> cru_quantity, std::string("")) 
  ).send(); 


  changes.modify(change, _me, [&](auto &c){
    c.status = "canceled"_n;
  });  

}

[[eosio::action]] void interchange::change(uint64_t id){
    require_auth(_changer);

    changes_index changes(_me, _me.value);
    auto change = changes.find(id);

    eosio::check(change != changes.end(), "Change object is not found");
    eosio::check(change -> status == "process"_n, "Change object is proccessed or canceled");

    action(
        permission_level{ _me, "active"_n },
        _token_contract, "retire"_n,
        std::make_tuple( _me, change -> cru_quantity, std::string("retire")) 
    ).send(); 


    action(
        permission_level{ _me, "active"_n },
        _token_contract, "transfer"_n,
        std::make_tuple( _me, change -> username, change -> wcru_quantity, std::string("")) 
    ).send(); 


    changes.modify(change, _changer, [&](auto &c){
      c.status = "success"_n;
    });


  }

 void interchange::create_change_object(eosio::name username, eosio::asset quantity){
    require_auth(username);
  
    changes_index changes(_me, _me.value);

    changes.emplace(_me, [&](auto &r){
      r.id = changes.available_primary_key();
      r.date = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());;
      r.username = username;
      r.cru_quantity = quantity;
      r.wcru_quantity = eosio::asset(quantity.amount, _wcru_symbol);
      r.status = "process"_n;
    });
    
  }


extern "C" {
   
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (code == interchange::_me.value) {
          if (action == "change"_n.value){
            execute_action(name(receiver), name(code), &interchange::change);
          } else if (action == "cancel"_n.value){
            execute_action(name(receiver), name(code), &interchange::cancel);
          }             
        } else {

          if (action == "transfer"_n.value){
            
            struct transfer {
                eosio::name from;
                eosio::name to;
                eosio::asset quantity;
                std::string memo;
            };

            auto op = eosio::unpack_action_data<transfer>();

            if (op.to == interchange::_me){
                if (op.from != interchange::_reserve){
                  eosio::check(name(code) == interchange::_token_contract, "Wrong token contract");
                  eosio::check(op.quantity.symbol == interchange::_cru_symbol, "Wrong symbol");

                  interchange::create_change_object(op.from, op.quantity);  
                }
                
            }
          }
        }
  };
};
