--技能讲解7：基本技能类型的组合和复杂技能的实现（二）
--[[
	在这个章节中，我将带领大家看一个复杂的技能。
	这个技能几乎综合了前面Lua中所说明的技巧。
	好了，话不多说，上技能：
]]

--[[
	略懂：(出牌阶段限一次)每当你需要使用或打出一张基本牌时，你可展示牌堆顶1+X张牌（X为你当前已损失体力值且至少为1）。你可使用或打出这些牌中一张相应类别的牌。（这些牌中，你可以将黑桃为【酒】，方块为【杀】，梅花为【闪】。）
]]
--[[
	没有什么特殊的值得分析的，需要说的都在代码里了。
]]
local json = require ("json")
--载入json库，我们可以使用json.encode来生成json字符串来操控json。
function view(room, player, ids, enabled, disabled,skill_name)
	local result = -1;
    local jsonLog = {
		"$ViewDrawPile",
		player:objectName(),
		"",
		table.concat(sgs.QList2Table(ids),"+"),
		"",
		""
	}
    room:doNotify(player,sgs.CommandType.S_COMMAND_LOG_SKILL, json.encode(jsonLog))
	----------doNotify--------
	--这个函数的作用是更新客户端
	--通过Json和相关的命令来更新客户端。
	--与它功能相同的还有doBroadcastNotify等
	--参数：
	--第一个参数是ServerPlayer类型，代指被更新客户端的玩家。
	--第二个参数是commandType类型（实质是int），更新客户端的命令或者说通讯协议
	--第三个参数是字符串类型，也就是新参数。
	--关于commandType可以参见lua\utilities.lua的最后部分
	--在源码中，一般通讯协议的表示方式是这样：
	--QSanProtocol::加上一个通讯协议
	--而在Lua中，我们只需要把QSanProtocol::换成sgs.CommandType.
	--对于第三个参数value，通常有两种形式：
	--一般的value
	--数组array
	--如果处理一般的value的话只需要用json.encode(value)就可以了
	--如果是数组array的话就需要完全按照源码中的顺序来写
	--有时候array里面还会再套一个从QList转化过来的array
	--我们只需要用这个函数sgs.QList2Table把它转成Lua table就可以了
	--最后别忘了用json.encode把table变成Json字符串
	--具体的例子我会在下面演示。
    room:notifySkillInvoked(player,skill_name)
	---这句话的作用是通知所有客户端某人使用了什么技能。
	--具体作用就是在玩家框上显示技能名。
    if enabled:isEmpty() then
		local jsonValue = {
            ".",
            false,
            sgs.QList2Table(ids)
		}
        room:doNotify(player,sgs.CommandType.S_COMMAND_SHOW_ALL_CARDS, json.encode(jsonValue))
		--在这个地方源码是这样写的，请对比一下。
		--[[
		Json::Value arg(Json::arrayValue);
        arg[0] = QSanProtocol::Utils::toJsonString(".");
        arg[1] = false;
        arg[2] = QSanProtocol::Utils::toJsonArray(ids);
        room->doNotify(player, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, arg);
		]]
		--可以说这种东西没有源码根本写不出来……
		--所以请不要擅自用json
		--尽可能的参考源码写。
    else
        room:fillAG(ids, player, disabled)
        local id = room:askForAG(player, enabled, true,skill_name);
        if (id ~= -1) then
            ids:removeOne(id)
            result = id
        end
        room:clearAG(player)
		--这里还是使用AG框来选牌
    end
	if ids:length() > 0 then
		local drawPile = room:getDrawPile() --新版的getDrawPile是对真正的Pile进行引用。
        for i = ids:length() - 1,0,-1 do
            drawPile:prepend(ids:at(i))
		end
		--把牌放回牌堆
        room:doBroadcastNotify(sgs.CommandType.S_COMMAND_UPDATE_PILE, sgs.QVariant(drawPile:length()))
		--通知客户端更新牌堆数目
	end
    if result == -1 then
        room:setPlayerFlag(player, "Global_"..skill_name.."Failed")
	end
    return result
end

devLvedongCard=sgs.CreateSkillCard{
	name="devLvedongCard",
	will_throw = false,
	skill_name = "devLvedong",
	filter = function(self, targets, to_select)
		local name = ""
		local card
		local plist = sgs.PlayerList()
		for i = 1, #targets do plist:append(targets[i]) end
		local aocaistring = self:getUserString()
		if aocaistring ~= "" then
			local uses = aocaistring:split("+")
			name = uses[1]
			card = sgs.Sanguosha:cloneCard(name)
		end
		card:deleteLater() --不用的要删除
		return card and card:targetFilter(plist, to_select, sgs.Self) --调用相关Card的相关函数
	end ,
	feasible = function(self, targets)
		local name = ""
		local card
		local plist = sgs.PlayerList()
		for i = 1, #targets do plist:append(targets[i]) end
		local aocaistring = self:getUserString()
		if aocaistring ~= "" then
			local uses = aocaistring:split("+")
			name = uses[1]
			card = sgs.Sanguosha:cloneCard(name)
		end
		card:deleteLater()
		return card and card:targetsFeasible(plist, sgs.Self)--同上
	end,
	on_validate_in_response = function(self, user) --在技能卡中介绍过
		local room = user:getRoom()
		local ids = room:getNCards(1 + math.max(user:getLostHp(),1), false)
		local aocaistring = self:getUserString()
		local names = aocaistring:split("+")
		if table.contains(names, "slash") then
			table.insert(names,"fire_slash")
			table.insert(names,"thunder_slash")
		end
		local filterFunction = function(id)
			local card = sgs.Sanguosha:getCard(id)
			if card == nil then return nil end
			if card:getSuit() == sgs.Card_Spade then
				return "analeptic"
			elseif card:getSuit() == sgs.Card_Club then
				return "jink"
			elseif card:getSuit() == sgs.Card_Diamond then
				return "slash"
			else
				return card:objectName()
			end
		end
		local enabled, disabled = sgs.IntList(), sgs.IntList()
		for _,id in sgs.qlist(ids) do
			if table.contains(names, filterFunction(id)) or table.contains(names,sgs.Sanguosha:getCard(id):objectName())  then
				enabled:append(id)
			else
				disabled:append(id)
			end
		end
		local id = view(room, user, ids, enabled, disabled,"devLvedong")
		if id == -1 then return nil end
		if table.contains(names,sgs.Sanguosha:getCard(id):objectName()) then
			return sgs.Sanguosha:getCard(id)
		else
			local returnCard = sgs.Sanguosha:cloneCard(filterFunction(id))
			if returnCard then 
				returnCard:addSubcard(id)
				returnCard:setSkillName("devLvedong")
			end
			return returnCard
		end
	end,
	on_validate = function(self, cardUse) --在技能卡中介绍过
		cardUse.m_isOwnerUse = false
		local user = cardUse.from
		local room = user:getRoom()
		local ids = room:getNCards(1 + math.max(user:getLostHp(),1), false)
		local aocaistring = self:getUserString()
		local names = aocaistring:split("+")
		if table.contains(names, "slash") then
			table.insert(names,"fire_slash")
			table.insert(names,"thunder_slash")
		end
		local filterFunction = function(id)
			local card = sgs.Sanguosha:getCard(id)
			if card == nil then return nil end
			if card:getSuit() == sgs.Card_Spade then
				return "analeptic"
			elseif card:getSuit() == sgs.Card_Club then
				return "jink"
			elseif card:getSuit() == sgs.Card_Diamond then
				return "slash"
			else
				return card:objectName()
			end
		end
		local enabled, disabled = sgs.IntList(), sgs.IntList()
		for _,id in sgs.qlist(ids) do
			if table.contains(names, filterFunction(id)) or table.contains(names,sgs.Sanguosha:getCard(id):objectName())  then
				enabled:append(id)
			else
				disabled:append(id)
			end
		end
		local id = view(room, user, ids, enabled, disabled,"devLvedong")
		if id == -1 then return nil end
		if table.contains(names,sgs.Sanguosha:getCard(id):objectName()) then
			return sgs.Sanguosha:getCard(id)
		else
			local returnCard = sgs.Sanguosha:cloneCard(filterFunction(id))
			if returnCard then 
				returnCard:addSubcard(id)
				returnCard:setSkillName("devLvedong")
			end
			return returnCard
		end
	end
}
devLvedongVS = sgs.CreateZeroCardViewAsSkill{
	name = "devLvedong",
	enabled_at_play = function(self, player)
		if player:hasFlag("Global_devLvedongFailed") or player:hasUsed("#devLvedongCard") then return false end
		return sgs.Slash_IsAvailable(player) or sgs.Analeptic_IsAvailable(player) or player:isWounded()
	end,
	enabled_at_response=function(self, player, pattern)
		if player:hasFlag("Global_devLvedongFailed") then return end
		if pattern == "slash" then
			return sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE
		elseif (pattern == "peach") then
			return not player:hasFlag("Global_PreventPeach")
		elseif string.find(pattern, "analeptic") then
			return true
		end
		return false
	end,
	view_as = function(self)
		local acard = devLvedongCard:clone()
		local pattern = "233"
		if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY then
			pattern = sgs.Self:getTag("devLvedong"):toString() --蛊惑框点完之后就会把选的牌的objectName储存在sgs.Self的名为该技能objectName的Tag中
		else
			pattern = sgs.Sanguosha:getCurrentCardUsePattern()
			if pattern == "peach+analeptic" and sgs.Self:hasFlag("Global_PreventPeach") then
				pattern = "analeptic"
			end
		end
		local ac = sgs.Sanguosha:cloneCard(pattern:split("+")[1])
		ac:deleteLater()
		acard:setTargetFixed(ac:targetFixed()) --设置TargetFixed
		acard:setUserString(pattern)
		acard:setShowSkill(self:objectName())
		acard:setSkillName(self:objectName())
		return acard
	end,
}
devLvedong = sgs.CreateTriggerSkill{
	name = "devLvedong" ,
	events = {sgs.CardAsked},
	view_as_skill = devLvedongVS,
	guhuo_type = "b", --蛊惑框类型，在视为技部分介绍过。
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() and player:hasSkill(self:objectName()) then
			local pattern = data:toStringList()[1]
			if pattern == "slash" or pattern == "jink" then
				return self:objectName()
			end
		end
		return ""
	end ,
	on_cost = function(self, event, room, player, data)
		if room:askForSkillInvoke(player,self:objectName(),data) then
			return true
		end
	end ,
	on_effect = function(self, evnet, room, player, data)
		local pattern = data:toStringList()[1]
		local ids = room:getNCards(1 + math.max(player:getLostHp(),1), false)
		local enabled, disabled = sgs.IntList(), sgs.IntList()
		local filterFunction = function(id)
			local card = sgs.Sanguosha:getCard(id)
			if card == nil then return nil end
			if card:getSuit() == sgs.Card_Spade then
				return "analeptic"
			elseif card:getSuit() == sgs.Card_Club then
				return "jink"
			elseif card:getSuit() == sgs.Card_Diamond then
				return "slash"
			else
				return card:objectName()
			end
		end
		for _,id in sgs.qlist(ids) do
			if string.find(filterFunction(id), pattern) or string.find(sgs.Sanguosha:getCard(id):objectName(), pattern) then
				enabled:append(id)
			else
				disabled:append(id)
			end
		end
		local id = view(room, player, ids, enabled, disabled,self:objectName())
		if id ~= -1 then
			if string.find(sgs.Sanguosha:getCard(id):objectName(), pattern) then
				room:provide(sgs.Sanguosha:getCard(id))
			else
				local returnCard = sgs.Sanguosha:cloneCard(filterFunction(id))
				if returnCard then 
					returnCard:addSubcard(id)
					returnCard:setSkillName(self:objectName())
				end
				room:provide(returnCard)
			end
			return true
		end
	end ,
}
--[[
	这个复杂的技能穿插了好多高级的东西和技巧，希望能给大家一个借鉴。
]]
--[[
	到这里，国战神杀扩展文档就告一段落了，希望这些简单的例子能够帮助大家解决问题。
	如果你有什么好的建议或者意见，可以联系数字。电子邮箱不知道在哪个文件……
]]